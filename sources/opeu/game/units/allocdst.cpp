// Distributes the allocated amounts over the units (or the objective), lets the player choose each unit's regimental
// headquarters, and finishes the allocation.
#include "types.hpp"
#include "units/allocdst.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/memory.hpp"
#include "units/allocpnl.hpp"
#include "common/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"
#include "core/chief.hpp"
#include "io/listdlg.hpp"
#include "io/waitkey.hpp"
#include "units/capacity.hpp"
#include "units/elemlink.hpp"
#include "units/release.hpp"
#include "units/resagg.hpp"
#include "units/supply.hpp"
#include "units/unitname.hpp"

using namespace opeu::game;

namespace {
// The services the module calls through its event (svc_allocdst). Capacities come back in args[1..3].
enum Service : std::uint16_t {
  REMOVE_ELEMENT       = 0,  // (unit, element)
  INSERT_ELEMENT       = 1,  // (unit, element)
  RELEASE_ELEMENTS     = 2,  // (list, mask)
  UNIT_CAPACITIES      = 3,  // (unit) -> args[1..3]
  OBJECTIVE_CAPACITIES = 4,  // (objective) -> args[1..3]
  SHARE_UNITS          = 5,  // (unit, other)
  SHARE_OBJECTIVE      = 6,  // (unit, objective)
  UNIT_NAME            = 7,  // (unit, buffer) -> text
  REQUEST_FORMAT       = 8,  // (buffer, format, text)
  CHIEF_TEXT           = 9,  // (text)
  LIST_DIALOG          = 10, // (dialog, mode, height)
  PENDING_FINISH       = 11, // (code)
};

// The DS stack frame of the supply exchange services.
constexpr std::uint16_t SUPPLY_FRAME{ 0x200u };
// The offset of opeu_tally_mark in the callback segment: the choice dialog's select callback.
constexpr std::uint16_t TALLY_MARK_OFFSET{ 0x10b8 };
// Stored resources are capped at this amount.
constexpr std::int32_t RESOURCE_LIMIT{ 60000 };

// Asks the event for the three capacities of a unit or an objective.
auto Capacities(std::uint8_t* ds, HostWordsEvent event, Service service,
                std::uint16_t owner, std::uint16_t& a, std::uint16_t& b,
                std::uint16_t& c) -> void {
  std::uint16_t args[4]{ owner, a, b, c };
  event(ds, service, args, 1);
  a = args[1];
  b = args[2];
  c = args[3];
}

// Copies an allocation row into a stock item, clearing its extra word.
auto Fill(Item& item, Item const& row) -> void {
  item.type     = row.type;
  item.quantity = row.quantity;
  item.extra    = 0;
}
} // namespace

// Rebuilds each unit's element list from its allocation rows (or fills the objective's six items), then splits the
// summed resources: the first unit up to its capacities, the rest to the second unit or (capped at 60000) to the
// objective.
void opeu_alloc_distribute(uint8_t ds[65536], HostWordsEvent event,
                           uint8_t first, uint8_t second, uint8_t third,
                           uint8_t count) {
  auto&         data{ Data(ds) };
  std::int32_t  sum_a{ 0 }, sum_b{ 0 }, sum_c{ 0 };
  std::uint16_t capacity_a[2]{}, capacity_b[2]{}, capacity_c[2]{};
  std::uint8_t  start{ 0 };
  for (std::uint8_t i{ 0 }; i < 2; ++i) {
    auto const unit_near{ data.alloc_units[i] };
    if (unit_near != 0) {
      auto& unit{ Record<Unit>(data, unit_near) };
      while (unit.first)
        Call(ds, event, REMOVE_ELEMENT, unit_near, unit.first);
      Call(ds, event, RELEASE_ELEMENTS, Near(data, data.alloc_elements), 0);
      std::uint8_t used{ 0 };
      for (auto row{ start }; row < first; ++row) {
        auto const& slot{ data.alloc_slots[row] };
        if (!slot.quantity)
          continue;
        auto const element_near{ data.alloc_elements[used++] };
        auto&      element{ Record<Element>(data, element_near) };
        element.type     = slot.type;
        element.quantity = slot.quantity;
        element.damage   = 0;
        opeu_alloc_element_values(ds, event, element_near, count);
        Call(ds, event, INSERT_ELEMENT, unit_near, element_near);
      }
      sum_a += unit.resources[0];
      sum_b += unit.resources[1];
      sum_c += unit.resources[2];
      Capacities(ds, event, UNIT_CAPACITIES, unit_near, capacity_a[i],
                 capacity_b[i], capacity_c[i]);
      start = first;
      first += second;
    } else {
      auto&        objective{ Record<Objective>(data, data.alloc_objective) };
      std::uint8_t used{ 0 };
      for (auto row{ start }; third + start > row; ++row)
        if (data.alloc_slots[row].quantity)
          Fill(objective.items[used++], data.alloc_slots[row]);
      for (std::uint8_t row{ 0 }; row < data.tally_reject_count; ++row)
        if (data.supply_spare[row].quantity)
          Fill(objective.items[used++], data.supply_spare[row]);
      for (; used < 6; ++used)
        objective.items[used].quantity = 0;
      sum_a += objective.resources[0];
      sum_b += objective.resources[1];
      sum_c += objective.resources[2];
      Capacities(ds, event, OBJECTIVE_CAPACITIES, data.alloc_objective,
                 capacity_a[1], capacity_b[1], capacity_c[1]);
    }
  }
  auto& unit{ Record<Unit>(data, data.alloc_units[0]) };
  unit.resources[0] = sum_a > std::int32_t{ capacity_a[0] }
                          ? capacity_a[0]
                          : static_cast<std::uint16_t>(sum_a);
  unit.resources[1] = sum_b > std::int32_t{ capacity_b[0] }
                          ? capacity_b[0]
                          : static_cast<std::uint16_t>(sum_b);
  unit.resources[2] = sum_c > std::int32_t{ capacity_c[0] }
                          ? capacity_c[0]
                          : static_cast<std::uint16_t>(sum_c);
  if (auto const other_near{ data.alloc_units[1] }; other_near != 0) {
    auto& other{ Record<Unit>(data, other_near) };
    other.resources[0] = static_cast<std::uint16_t>(sum_a) - unit.resources[0];
    other.resources[1] = static_cast<std::uint16_t>(sum_b) - unit.resources[1];
    other.resources[2] = static_cast<std::uint16_t>(sum_c) - unit.resources[2];
    Call(ds, event, SHARE_UNITS, data.alloc_units[0], other_near);
  } else {
    auto& objective{ Record<Objective>(data, data.alloc_objective) };
    std::int32_t const sums[3]{ sum_a, sum_b, sum_c };
    for (auto resource{ 0 }; resource < 3; ++resource) {
      if (sums[resource] - unit.resources[resource] > RESOURCE_LIMIT)
        objective.resources[resource] = RESOURCE_LIMIT;
      else
        objective.resources[resource] =
            static_cast<std::uint16_t>(sums[resource]) -
            unit.resources[resource];
    }
    Call(ds, event, SHARE_OBJECTIVE, data.alloc_units[0], data.alloc_objective);
  }
}

// For each present unit, lists its elements in the choice dialog and asks for the regimental headquarters. `text` and
// `name` are the caller's 100- and 32-byte buffers; the dialog's select callback is opeu_tally_mark.
void opeu_alloc_choose_hq(uint8_t ds[65536], HostWordsEvent event,
                          uint16_t text, uint16_t name,
                          uint16_t callback_segment) {
  auto& data{ Data(ds) };
  for (std::uint8_t i{ 0 }; i < 2; ++i) {
    auto const unit_near{ data.alloc_units[i] };
    if (unit_near == 0)
      continue;
    std::uint8_t listed{ 0 };
    for (auto it{ Elements(data, Record<Unit>(data, unit_near)).begin() };
         it != std::default_sentinel; ++it)
      data.alloc_elements[listed++] = it.Near();
    data.choice_dialog.rows   = static_cast<std::uint8_t>(listed + 1);
    data.choice_dialog.select = { TALLY_MARK_OFFSET, callback_segment };
    Call(ds, event, REQUEST_FORMAT, text, Near(data, data.regiment_hq_prompt),
         Call(ds, event, UNIT_NAME, unit_near, name));
    Call(ds, event, CHIEF_TEXT, text);
    Call(ds, event, LIST_DIALOG, Near(data, data.choice_dialog), 1, 9);
  }
}

void opeu_alloc_finish(uint8_t ds[65536], HostWordsEvent event) {
  Call(ds, event, PENDING_FINISH, 3);
}

// The module's services: element relinking and release, capacities, the supply exchanges, unit names, the prompt, the
// chief's text, the list dialog and the pending finish.
uint16_t svc_allocdst(uint8_t* ds, uint16_t id, uint16_t* args,
                      uint16_t count) {
  uint16_t sp, r = 0;
  switch (id) {
    case 0:
      r = opeu_remove_element(ds, Arg(args, count, 0), Arg(args, count, 1));
      break;
    case 1:
      opeu_insert_element(ds, Arg(args, count, 0), Arg(args, count, 1));
      break;
    case 2:
      r = opeu_release_side_elements(ds, Arg(args, count, 0),
                                     (uint8_t)Arg(args, count, 1));
      break;
    case 3:
      opeu_unit_capacities(ds, Arg(args, count, 0), &args[1], &args[2],
                           &args[3]);
      break;
    case 4:
      opeu_objective_capacities(ds, Arg(args, count, 0), &args[1], &args[2],
                                &args[3]);
      break;
    case 5: // the two 32-byte unit-name buffers live in a DS stack frame
      sp = port_stack_enter(SUPPLY_FRAME);
      opeu_supply_exchange_units(
          ds, svc_supply, Arg(args, count, 0), Arg(args, count, 1),
          (uint16_t)(sp - 2u - 0x20u), (uint16_t)(sp - 2u - 0x40u));
      port_stack_leave(SUPPLY_FRAME);
      break;
    case 6:
      sp = port_stack_enter(SUPPLY_FRAME);
      opeu_supply_exchange_objective(ds, svc_supply, Arg(args, count, 0),
                                     Arg(args, count, 1),
                                     (uint16_t)(sp - 2u - 0x20u));
      port_stack_leave(SUPPLY_FRAME);
      break;
    case 7:
      r = opeu_unit_name(ds, svc_unitname, Arg(args, count, 0),
                         Arg(args, count, 1));
      break;
    case 8: r = lf_request_format(ds, args, count); break;
    case 9: opeu_ops_chief_text(ds, svc_chief, Arg(args, count, 0)); break;
    case 10:
      r = opeu_list_dialog(ds, svc_listdlg, Arg(args, count, 0),
                           (uint8_t)Arg(args, count, 1),
                           (uint8_t)Arg(args, count, 2));
      break;
    case 11:
      opeu_pending_finish(ds, svc_waitkey, (uint8_t)Arg(args, count, 0));
      break;
    default: port_unhandled("allocdst", id, count);
  }
  return r;
}

auto port_routine_opeu_alloc_finish(std::uint8_t* ds, std::uint16_t const* args,
                                    unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_alloc_finish(ds, svc_allocdst);
  return 0;
}
