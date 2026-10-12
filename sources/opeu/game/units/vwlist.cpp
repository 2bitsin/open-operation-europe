// The information views' list builders and the row checks of their list dialogs.
#include "types.hpp"
#include "units/vwlist.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/memory.hpp"
#include "common/view-service.hpp"
#include "common/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"
#include "core/recindex.hpp"
#include "core/selunit.hpp"
#include "map/objeleg.hpp"
#include "map/query.hpp"
#include "map/support.hpp"

using namespace opeu::game;

namespace {
auto Send(std::uint8_t* ds, HostEvent event, ViewService service, auto... words)
    -> std::uint16_t {
  return Call(ds, event, service, words...);
}
} // namespace

// Lists the named objectives `side` holds in view_objectives; returns their count.
uint8_t opeu_list_side_objectives(uint8_t ds[65536], HostEvent event,
                                  uint16_t sp, uint8_t side) {
  (void)sp;
  auto&        data{ Data(ds) };
  std::uint8_t count{ 0 };
  for (auto const& objective : data.objectives)
    if (objective.active &&
        static_cast<std::uint8_t>(Send(ds, event, ViewService::OBJECTIVE_SIDE,
                                       Near(data, objective))) == side)
      data.view_objectives[count++] = Near(data, objective);
  return count;
}

// Lists a regiment's elements in the element rows; returns their count.
uint8_t opeu_list_unit_elements(uint8_t ds[65536], HostEvent event, uint16_t sp,
                                uint16_t unit) {
  (void)event;
  (void)sp;
  auto&        data{ Data(ds) };
  std::uint8_t count{ 0 };
  auto const   chain{ Elements(data, Record<Unit>(data, unit)) };
  for (auto it{ chain.begin() }; it != chain.end(); ++it)
    data.row_elements[count++] = it.Near();
  return count;
}

// Lists an objective's stock items that hold an amount in view_stocks; returns their count.
uint8_t opeu_list_objective_stocks(uint8_t ds[65536], HostEvent event,
                                   uint16_t sp, uint16_t objective) {
  (void)event;
  (void)sp;
  auto&        data{ Data(ds) };
  std::uint8_t count{ 0 };
  for (auto const& item : Record<Objective>(data, objective).items)
    if (item.quantity)
      data.view_stocks[count++] = Near(data, item);
  return count;
}

// The total quantity of a regiment's spotted elements whose type is of `category`; their number goes to the byte at
// `count`.
uint16_t opeu_element_kind_total(uint8_t ds[65536], HostEvent event,
                                 uint16_t sp, uint16_t unit, uint8_t category,
                                 uint16_t count) {
  (void)event;
  (void)sp;
  auto&         data{ Data(ds) };
  std::uint16_t total{ 0 };
  Byte(ds, count) = 0;
  for (auto const& element : Elements(data, Record<Unit>(data, unit)))
    if (data.element_types[element.type].category == category &&
        (element.flags & Bits(ElementFlag::SPOTTED))) {
      total += element.quantity;
      ++Byte(ds, count);
    }
  return total;
}

// Row check: an own objective that may be listed, or an enemy objective that was reconnoitred.
uint8_t opeu_view_objective_ok(uint8_t ds[65536], HostEvent event,
                               uint16_t sp) {
  (void)sp;
  auto&      data{ Data(ds) };
  auto const near{ data.view_objectives[data.list_row] };
  auto const side{ static_cast<std::uint8_t>(
      Send(ds, event, ViewService::OBJECTIVE_SIDE, near)) };
  return (side == data.current_side &&
          Send(ds, event, ViewService::OBJECTIVE_ELIGIBLE, near)) ||
         (side != data.current_side &&
          (Record<Objective>(data, near).flags & Bits(ObjectiveFlag::MARKED)));
}

// Row check that rejects every row.
uint8_t opeu_view_false(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  (void)ds;
  (void)event;
  (void)sp;
  return 0;
}

// Row check: an active division whose selected regiment may be selected.
uint8_t opeu_view_group_ok(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  (void)sp;
  auto&       data{ Data(ds) };
  auto const& division{
    data.groups[data.current_side * GROUPS_PER_SIDE + data.list_row]
  };
  return (division.flags & Bits(GroupFlag::READY)) &&
         Send(ds, event, ViewService::UNIT_ELIGIBLE,
              Send(ds, event, ViewService::GROUP_SELECTED_UNIT,
                   Near(data, division)));
}

// Row check: a ready regiment, selectable if it is ours, spotted if it is the enemy's.
uint8_t opeu_view_unit_ok(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  (void)sp;
  auto&       data{ Data(ds) };
  auto const  unit{ data.row_units[data.list_row] };
  auto const& regiment{ Record<Unit>(data, unit) };
  auto const  side{ static_cast<std::uint8_t>(
      Send(ds, event, ViewService::UNIT_SIDE, unit)) };
  return (regiment.flags & Bits(UnitFlag::READY)) &&
         ((side == data.current_side &&
           Send(ds, event, ViewService::UNIT_ELIGIBLE, unit)) ||
          (side != data.current_side &&
           (regiment.flags & Bits(UnitFlag::SPOTTED))));
}

// Further row checks that reject every row.
uint8_t opeu_view_false_2(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  return opeu_view_false(ds, event, sp);
}

uint8_t opeu_view_false_3(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  return opeu_view_false(ds, event, sp);
}

// Row check that accepts every row.
uint8_t opeu_view_true(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  (void)ds;
  (void)event;
  (void)sp;
  return 1;
}

// Appends to the word list at `list` the equipment types of `category` that a nation in `mask` fields and that are in
// service by the current month (the type's date is year * 16 + month); returns how many were appended.
uint8_t opeu_list_element_types(uint8_t ds[65536], HostEvent event, uint16_t sp,
                                uint16_t list, uint8_t mask, uint8_t category) {
  (void)event;
  (void)sp;
  auto& data{ Data(ds) };
  auto  at{ list };
  for (auto const& type : data.element_types)
    if ((type.sides & mask) && type.category == category &&
        (data.year << 4) + data.month >= type.date) {
      SetWord(ds, at, Near(data, type));
      at = static_cast<std::uint16_t>(at + 2);
    }
  return static_cast<std::uint8_t>(
      static_cast<std::int16_t>(static_cast<std::uint16_t>(at - list)) >> 1);
}

// The views' calls this module makes.
uint16_t svc_vwlist(uint8_t* ds, uint16_t id, const uint16_t* args,
                    uint16_t count) {
  uint16_t r = 0;
  switch (static_cast<ViewService>(id)) {
    case ViewService::OBJECTIVE_SIDE:
      r = opeu_objective_side(ds, Arg(args, count, 0));
      break;
    case ViewService::OBJECTIVE_ELIGIBLE:
      r = opeu_objective_eligible(ds, Arg(args, count, 0));
      break;
    case ViewService::GROUP_SELECTED_UNIT:
      r = opeu_find_group_selected_unit(ds, Arg(args, count, 0));
      break;
    case ViewService::UNIT_ELIGIBLE:
      r = opeu_unit_selection_eligible(ds, Arg(args, count, 0));
      break;
    case ViewService::UNIT_SIDE: r = opeu_unit_side(Arg(args, count, 0)); break;
    default:                     port_unhandled("vwlist", id, count);
  }
  return r;
}

auto port_routine_opeu_view_false(std::uint8_t* ds, std::uint16_t const* args,
                                  unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return WithStack(PORT_FRAME, [&](std::uint16_t sp) {
    return opeu_view_false(ds, svc_vwlist, sp);
  });
}

auto port_routine_opeu_view_false_2(std::uint8_t* ds, std::uint16_t const* args,
                                    unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return WithStack(PORT_FRAME, [&](std::uint16_t sp) {
    return opeu_view_false_2(ds, svc_vwlist, sp);
  });
}

auto port_routine_opeu_view_false_3(std::uint8_t* ds, std::uint16_t const* args,
                                    unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return WithStack(PORT_FRAME, [&](std::uint16_t sp) {
    return opeu_view_false_3(ds, svc_vwlist, sp);
  });
}

auto port_routine_opeu_view_group_ok(std::uint8_t*        ds,
                                     std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  (void)args;
  (void)count;
  return WithStack(PORT_FRAME, [&](std::uint16_t sp) {
    return opeu_view_group_ok(ds, svc_vwlist, sp);
  });
}

auto port_routine_opeu_view_objective_ok(std::uint8_t*        ds,
                                         std::uint16_t const* args,
                                         unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return WithStack(PORT_FRAME, [&](std::uint16_t sp) {
    return opeu_view_objective_ok(ds, svc_vwlist, sp);
  });
}

auto port_routine_opeu_view_true(std::uint8_t* ds, std::uint16_t const* args,
                                 unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return WithStack(PORT_FRAME, [&](std::uint16_t sp) {
    return opeu_view_true(ds, svc_vwlist, sp);
  });
}

auto port_routine_opeu_view_unit_ok(std::uint8_t* ds, std::uint16_t const* args,
                                    unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return WithStack(PORT_FRAME, [&](std::uint16_t sp) {
    return opeu_view_unit_ok(ds, svc_vwlist, sp);
  });
}
