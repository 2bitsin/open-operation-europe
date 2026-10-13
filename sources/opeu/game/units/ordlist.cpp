// Orders command (turn action 1): the player picks divisions of the side in turn. A division the computer runs gets
// its objective tasks edited; otherwise the player orders the whole division at once, or picks its regiments one by
// one for the order menu.
#include "units/ordlist.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/forces.hpp"
#include "common/memory.hpp"
#include "core/people.hpp"
#include "map/ordtgt.hpp"
#include "types.hpp"
#include "units/order-dialog.hpp"
#include "units/ordunit.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/ordact.hpp"

using namespace opeu::game;

namespace {
// The link-table addresses of the list predicates in the Orders command's code segment.
enum Routine : std::uint16_t {
  GROUP_TARGET_ENABLED = 0x0006,
  MENU_ALWAYS          = 0x0040,
  SUPPLY_UNIT_ENABLED  = 0x021c,
};
// The Orders command's code segment in the routine link table.
constexpr std::uint16_t ORDERS_SEGMENT{ 0x1c23 };

auto RunList(std::uint8_t* ds, HostEvent event, ListWindow& list) -> std::uint16_t {
  return Call(ds, event, ORDERS_LIST_DIALOG, Near(Data(ds), list), LIST_LIMIT, LIST_FLAGS);
}
} // namespace

// Lists the eight regiments of the current side's division `group` in the regiment rows.
uint8_t opeu_order_units_fill(uint8_t ds[65536], uint8_t group) {
  auto& data{ Data(ds) };
  for (std::uint8_t index{ 0 }; index < 8; ++index) data.row_units[index] = Near(data, data.units[(data.current_side * GROUPS_PER_SIDE + group) * 8 + index]);
  return 8;
}

// The Orders command; segment is its code segment.
void opeu_turn_action_1(uint8_t ds[65536], HostEvent event, uint16_t segment) {
  auto& data{ Data(ds) };
  Call(ds, event, ORDERS_CONTROL_PANEL);
  data.unit_order_active = 0;
  for (;;) {
    SetListPredicate(data.division_dialog, GROUP_TARGET_ENABLED, segment);
    Call(ds, event, ORDERS_CONTROL_MESSAGE, Near(data, data.select_division), data.context_record2);
    if (!RunList(ds, event, data.division_dialog)) return;
    auto const group{ data.list_row };
    auto&      division{ GroupOf(data, data.current_side, group) };
    auto const person{ opeu_find_group_record(ds, data.current_side, group) };
    Call(ds, event, ORDERS_GROUP_SHOW, Near(data, division), group);
    if (division.flags & Bits(GroupFlag::AI_MARK)) {
      // The computer runs the division: edit its objective tasks, with the division current meanwhile.
      auto const saved{ data.current_group };
      data.current_group = group;
      SetListPredicate(data.order_target_list, MENU_ALWAYS, segment);
      for (;;) {
        Call(ds, event, ORDERS_CONTROL_MESSAGE, Near(data, data.select_order), person);
        if (!RunList(ds, event, data.order_target_list)) break;
        opeu_group_set_target(ds, event, group);
      }
      data.current_group = saved;
    } else if (Call(ds, event, ORDERS_CONFIRM_MESSAGE, Near(data, data.order_whole_division), person)) {
      data.unit_order_active = 1;
      Call(ds, event, ORDERS_UNIT_ORDER_MENU, group);
      data.unit_order_active = 0;
    } else {
      data.organize_list.rows = static_cast<std::uint8_t>(opeu_order_units_fill(ds, group) + 1);
      SetListPredicate(data.organize_list, SUPPLY_UNIT_ENABLED, segment);
      for (;;) {
        Call(ds, event, ORDERS_CONTROL_MESSAGE, Near(data, data.select_regiment), person);
        if (!RunList(ds, event, data.organize_list)) break;
        Call(ds, event, ORDERS_UNIT_ORDER_MENU, group);
      }
    }
  }
}

// The command's calls, and those opeu_group_set_target raises through its event, are order dialog services.
uint16_t svc_ordlist(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case ORDERS_REQUEST_FORMAT:
  case ORDERS_CONTROL_MESSAGE:
  case ORDERS_CONFIRM_MESSAGE:
  case ORDERS_ARRIVAL_LOCATION:
  case ORDERS_PICK_LOCATION:
  case ORDERS_CONTROL_PANEL:
  case ORDERS_GROUP_SHOW:
  case ORDERS_LIST_DIALOG:
  case ORDERS_UNIT_ORDER_MENU:  svc_orders(ds, id, args, count, &r); break;
  default:                      port_unhandled("ordlist", id, count);
  }
  return r;
}

auto port_routine_opeu_turn_action_1(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_turn_action_1(ds, svc_ordlist, ORDERS_SEGMENT);
  return 0;
}
