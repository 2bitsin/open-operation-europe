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
#include "core/dialogs.hpp"
#include "core/people.hpp"
#include "core/textout.hpp"
#include "io/keyboard.hpp"
#include "io/waitkey.hpp"
#include "map/mappick.hpp"
#include "map/mappos.hpp"
#include "map/mapunit.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/order-dialog.hpp"
#include "units/orders.hpp"
#include "units/ordunit.hpp"
#include "units/stats.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/ordact.hpp"

using namespace opeu::game;

namespace {
// The locals the services read and write, as offsets below the top of the routine's reserve.
constexpr std::uint16_t LOCALS_BYTES{ 0x7c };
enum ServiceLocal : std::uint16_t {
  BUFFER = 0x7c, // the message text
};
// The order menu of a whole division.
constexpr std::uint8_t DIVISION_MENU{ 3 };
// Where the order menu's speaker is shown.
constexpr std::uint16_t MENU_X{ 0x36 };
constexpr std::uint16_t MENU_Y{ 0x144 };
// The ticks the screen waits after an order.
constexpr std::uint16_t ORDER_DELAY{ 0x14 };
// The value of the cached control cell that forces a redraw.
constexpr std::uint8_t NO_CELL{ 0xff };

} // namespace

// group: the division (of the current side) the order is for.
void opeu_unit_order_menu(Game& game, uint8_t group) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ LOCALS_BYTES };
  auto const          local{ [&](ServiceLocal offset) { return locals.At(static_cast<std::uint16_t>(LOCALS_BYTES - offset)); } };
  GridCell            cell{};
  auto const          buffer{ local(BUFFER) };
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
  opeu_arrival_location(game, static_cast<uint8_t>(regiment.x), static_cast<uint8_t>(regiment.y), static_cast<uint8_t>(side));
  auto const person{ opeu_find_group_record(game, side, group) };
  if (!whole) opeu_unit_announce(game, unit);
  auto const&  prompts{ data.issue_prompts };
  std::uint8_t kind{};
  for (;;) {
    opeu_speaker_panel(game, person, 1);
    auto const menu{ whole ? DIVISION_MENU : OrderMenuKind(data.list_row) };
    auto const count{ OrderCount(data.issue_order_codes[menu]) };
    opeu_order_layout(game, menu);
    opeu_army_position(game, static_cast<int16_t>(MENU_X), static_cast<int16_t>(MENU_Y));
    auto const choice{ opeu_prompt_value_cancel(game, Near(data, prompts.your_orders), count, Near(data, data.order_menus[menu])) };
    opeu_speaker_panel(game, person, 0);
    if (choice == MENU_CANCEL) {
      opeu_cancel_unit_orders(game, unit, person);
      return;
    }
    kind = data.issue_order_codes[menu][choice - 1];
    if (kind == Kind(UnitOrder::REPAIR_CITY) && !UnitQuantity(data, unit, Bits(Category::REPAIR)).total) {
      opeu_control_message(game, Near(data, prompts.no_repair_unit), person);
      continue;
    }
    break;
  }
  std::uint16_t target{};
  for (;;) {
    cell = { regiment.x, regiment.y };
    if (kind != Kind(UnitOrder::REST) || !whole) {
      opeu_control_message(game, Near(data, prompts.select_target), person);
      auto const picked{ PickLocation(game, cell) };
      if (!picked) {
        opeu_cancel_unit_orders(game, unit, person);
        return;
      }
      cell = *picked;
    }
    if (CheckOrderTarget(game, prompts, buffer, person, unit, cell.x, cell.y, kind, target)) break;
    opeu_control_message(game, buffer, person);
    opeu_control_position(game, regiment.x, regiment.y);
    opeu_arrival_location(game, static_cast<uint8_t>(regiment.x), static_cast<uint8_t>(regiment.y), static_cast<uint8_t>(side));
  }
  AnnounceOrder(game, prompts, buffer, person, kind);
  if (!whole)
    opeu_set_unit_order(game, unit, { .target = target, .cell = cell, .save_current = false, .code = kind });
  else
    for (std::uint8_t index{ 0 }; index < 8; ++index) {
      auto&      member{ division_units(index) };
      auto const near{ Near(data, member) };
      if (!(member.flags & Bits(UnitFlag::READY)) || !opeu_unit_selection_eligible(game, near)) continue;
      if (kind == Kind(UnitOrder::REST)) {
        cell.x = member.x;
        cell.y = member.y;
      }
      // A regiment does not pursue or supply itself.
      auto const own_cell{ member.x == cell.x && member.y == cell.y };
      if (!((kind == Kind(UnitOrder::PURSUE) || kind == Kind(UnitOrder::SUPPLY_UNIT)) && own_cell))
        opeu_set_unit_order(game, near, { .target = target, .cell = cell, .save_current = false, .code = kind });
    }
  opeu_delay_units(game, static_cast<uint8_t>(ORDER_DELAY));
  data.overview_side = NO_CELL;
  data.view_side     = NO_CELL;
}
