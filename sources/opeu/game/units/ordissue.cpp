// Orders command (turn action 1): order one regiment, the listed one, or, while unit_order_active is set, the whole
// division (from its last selected regiment). The order menu repeats until an order is chosen (a repair needs repair
// crews); then a target cell is picked (rest in division mode keeps each regiment's own cell) and checked against the
// order. A refused target explains why and is picked again; an accepted order is announced and stored in the regiment,
// or in every ready, selectable regiment of the division. The picked cell and the message text stay in the data
// segment, where the services read and write them.
#include "units/ordissue.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/order.hpp"
#include "core/people.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/order-dialog.hpp"
#include "units/orders.hpp"
#include "units/ordunit.hpp"
#include "units/stats.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/ordact.hpp"

using namespace opeu::game;

namespace {
// The locals the services read and write, as offsets below the top of the routine's reserve.
constexpr std::uint16_t LOCALS_BYTES{ 0x7c };
enum ServiceLocal : std::uint16_t {
  PICKED_X = 0x0e,
  PICKED_Y = 0x12,
  BUFFER   = 0x7c, // the message text
};
// The order menu of a whole division.
constexpr std::uint8_t DIVISION_MENU{ 3 };
// Where the order menu's speaker is shown.
constexpr std::uint16_t MENU_X{ 0x36 };
constexpr std::uint16_t MENU_Y{ 0x144 };
// unit_quantity's category of repair crews.
constexpr std::uint8_t REPAIR_CREWS{ 6 };
// The ticks the screen waits after an order.
constexpr std::uint16_t ORDER_DELAY{ 0x14 };
// The value of the cached control cell that forces a redraw.
constexpr std::uint8_t NO_CELL{ 0xff };

} // namespace

// group: the division (of the current side) the order is for.
void opeu_unit_order_menu(Game& game, HostEvent event, uint8_t group) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ LOCALS_BYTES };
  auto const          local{ [&](ServiceLocal offset) { return locals.At(static_cast<std::uint16_t>(LOCALS_BYTES - offset)); } };
  auto const          x_at{ local(PICKED_X) };
  auto const          y_at{ local(PICKED_Y) };
  auto const          buffer{ local(BUFFER) };
  std::uint8_t        crews{};
  auto const          side{ data.current_side };
  auto const          whole{ data.unit_order_active != 0 };
  auto const          division_units{ [&](std::uint8_t index) -> Unit& { return data.units[(side * GROUPS_PER_SIDE + group) * 8 + index]; } };
  auto                unit{ data.row_units[data.list_row] };
  if (whole) {
    // The last regiment with the selection flag; without one, the listed regiment.
    for (std::uint8_t index{ 0 }; index < 8; ++index)
      if (division_units(index).flags & Bits(UnitFlag::SELECTED)) unit = Near(data, division_units(index));
  }
  auto& regiment{ Record<Unit>(data, unit) };
  if (!(regiment.flags & Bits(UnitFlag::READY)) || !opeu_unit_selection_eligible(game, unit)) return;
  Call(game, event, ORDERS_ARRIVAL_LOCATION, regiment.x, regiment.y, side);
  auto const person{ opeu_find_group_record(game, side, group) };
  if (!whole) Call(game, event, ORDERS_UNIT_ANNOUNCE, unit, group);
  auto const&  prompts{ data.issue_prompts };
  std::uint8_t kind{};
  for (;;) {
    Call(game, event, ORDERS_SPEAKER_PANEL, person, 1);
    auto const menu{ whole ? DIVISION_MENU : OrderMenuKind(data.list_row) };
    auto const count{ OrderCount(data.issue_order_codes[menu]) };
    opeu_order_layout(game, menu);
    Call(game, event, ORDERS_ARMY_POSITION, MENU_X, MENU_Y);
    auto const choice{ Call(game, event, ORDERS_PROMPT, Near(data, prompts.your_orders), count, Near(data, data.order_menus[menu])) };
    Call(game, event, ORDERS_SPEAKER_PANEL, person, 0);
    if (choice == MENU_CANCEL) {
      opeu_cancel_unit_orders(game, event, unit, person);
      return;
    }
    kind = data.issue_order_codes[menu][choice - 1];
    if (kind == Kind(UnitOrder::REPAIR_CITY) && !opeu_unit_quantity(game, unit, REPAIR_CREWS, &crews)) {
      Call(game, event, ORDERS_CONTROL_MESSAGE, Near(data, prompts.no_repair_unit), person);
      continue;
    }
    break;
  }
  std::uint16_t target{};
  for (;;) {
    Byte(game, x_at) = regiment.x;
    Byte(game, y_at) = regiment.y;
    if (kind != Kind(UnitOrder::REST) || !whole) {
      Call(game, event, ORDERS_CONTROL_MESSAGE, Near(data, prompts.select_target), person);
      if (!Call(game, event, ORDERS_PICK_LOCATION, x_at, y_at)) {
        opeu_cancel_unit_orders(game, event, unit, person);
        return;
      }
    }
    if (CheckOrderTarget(game, event, prompts, buffer, person, unit, Byte(game, x_at), Byte(game, y_at), kind, target)) break;
    Call(game, event, ORDERS_CONTROL_MESSAGE, buffer, person);
    Call(game, event, ORDERS_CONTROL_POSITION, regiment.x, regiment.y);
    Call(game, event, ORDERS_ARRIVAL_LOCATION, regiment.x, regiment.y, side);
  }
  AnnounceOrder(game, event, prompts, buffer, person, kind);
  if (!whole)
    opeu_set_unit_order(game, unit, target, Byte(game, x_at), 0, kind, Byte(game, y_at));
  else
    for (std::uint8_t index{ 0 }; index < 8; ++index) {
      auto&      member{ division_units(index) };
      auto const near{ Near(data, member) };
      if (!(member.flags & Bits(UnitFlag::READY)) || !opeu_unit_selection_eligible(game, near)) continue;
      if (kind == Kind(UnitOrder::REST)) {
        Byte(game, x_at) = member.x;
        Byte(game, y_at) = member.y;
      }
      // A regiment does not pursue or supply itself.
      auto const own_cell{ member.x == Byte(game, x_at) && member.y == Byte(game, y_at) };
      if (!((kind == Kind(UnitOrder::PURSUE) || kind == Kind(UnitOrder::SUPPLY_UNIT)) && own_cell)) opeu_set_unit_order(game, near, target, Byte(game, x_at), 0, kind, Byte(game, y_at));
    }
  Call(game, event, ORDERS_DELAY_UNITS, ORDER_DELAY);
  data.overview_side = NO_CELL;
  data.view_side     = NO_CELL;
}

// The menu's calls are order dialog services; confirm_message comes from opeu_cancel_unit_orders, which raises it
// through this event.
uint16_t svc_ordissue(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case ORDERS_REQUEST_FORMAT:
  case ORDERS_CONTROL_MESSAGE:
  case ORDERS_CONFIRM_MESSAGE:
  case ORDERS_ARRIVAL_LOCATION:
  case ORDERS_PICK_LOCATION:
  case ORDERS_SPEAKER_PANEL:
  case ORDERS_UNIT_ANNOUNCE:
  case ORDERS_ARMY_POSITION:
  case ORDERS_PROMPT:
  case ORDERS_CONTROL_POSITION:
  case ORDERS_MESSAGE_LOOKUP:
  case ORDERS_DELAY_DIALOG:
  case ORDERS_DELAY_UNITS:      svc_orders(game, id, args, count, &r); break;
  default:                      port_unhandled("ordissue", id, count);
  }
  return r;
}
