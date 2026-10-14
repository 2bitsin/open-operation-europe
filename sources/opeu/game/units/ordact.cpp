// The map cursor's unit action: a ready, selectable regiment becomes the listed row; the order menu repeats until an
// order is chosen (a repair needs repair crews), then a target cell is picked and checked like in the Orders command.
// The accepted order is announced and stored, the side's daily context refreshed and the screen waits; leaving the
// menu cancels pending orders after confirmation. Both exits but a cancelled pick reset the cached control cell.
#include "units/ordact.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/order.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/order-dialog.hpp"
#include "units/orders.hpp"
#include "units/ordunit.hpp"
#include "units/stats.hpp"

#include "common/game.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "core/textout.hpp"
#include "io/keyboard.hpp"
#include "io/listdlg.hpp"
#include "io/waitkey.hpp"
#include "map/mappick.hpp"
#include "map/mappos.hpp"
#include "map/mapunit.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/ordissue.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {
// The message text the services format: the room it had in the routine's frame.
constexpr std::size_t MESSAGE_BYTES{ 0x74 };
// Where the order menu's speaker is shown.
constexpr std::uint16_t MENU_X{ 0x36 };
constexpr std::uint16_t MENU_Y{ 0x144 };
// The ticks the screen waits after an order.
constexpr std::uint16_t ORDER_DELAY{ 0xf };
// The value of the cached control cell that forces a redraw.
constexpr std::uint8_t NO_CELL{ 0xff };

} // namespace

// unit: the regiment under the cursor.
void opeu_unit_action(Game& game, uint16_t unit, uint16_t /*unused*/) {
  auto&                     data{ Data(game) };
  GridCell                  cell{};
  TextBuffer<MESSAGE_BYTES> buffer;
  auto&                     regiment{ Record<Unit>(data, unit) };
  if (!(regiment.flags & Bits(UnitFlag::READY)) || !opeu_unit_selection_eligible(game, unit)) return;
  auto const person{ opeu_find_group_record(game, data.current_side, opeu_unit_group_index(unit)) };
  data.list_row = opeu_unit_index(unit);
  opeu_title_panel_begin(game);
  auto const&  prompts{ data.action_prompts };
  std::uint8_t kind{};
  auto const   done{ [&] {
    data.overview_side = NO_CELL;
    data.view_side     = NO_CELL;
  } };
  for (;;) {
    opeu_control_message(game, Near(data, data.awaiting_orders), person);
    opeu_speaker_panel(game, person, 1);
    auto const menu{ OrderMenuKind(data.list_row) };
    auto const count{ OrderCount(data.action_order_codes[menu]) };
    opeu_order_layout(game, menu);
    opeu_army_position(game, static_cast<int16_t>(MENU_X), static_cast<int16_t>(MENU_Y));
    auto const choice{ opeu_prompt_value_cancel(game, Near(data, prompts.your_orders), count, Near(data, data.order_menus[menu])) };
    opeu_speaker_panel(game, person, 0);
    if (choice == MENU_CANCEL) {
      opeu_cancel_unit_orders(game, unit, person);
      done();
      return;
    }
    kind = data.action_order_codes[menu][choice - 1];
    if (kind == Kind(UnitOrder::REPAIR_CITY) && !UnitQuantity(data, unit, Bits(Category::REPAIR)).total) {
      opeu_control_message(game, Near(data, prompts.no_repair_unit), person);
      continue;
    }
    break;
  }
  std::uint16_t target{};
  for (;;) {
    cell = { regiment.x, regiment.y };
    opeu_control_message(game, Near(data, prompts.select_target), person);
    auto const picked{ PickLocation(game, cell) };
    if (!picked) {
      opeu_cancel_unit_orders(game, unit, person);
      return;
    }
    cell = *picked;
    if (CheckOrderTarget(game, prompts, buffer, person, unit, cell.x, cell.y, kind, target)) break;
    opeu_control_message(game, buffer, person);
    opeu_control_position(game, regiment.x, regiment.y);
    opeu_arrival_location(game, static_cast<uint8_t>(regiment.x), static_cast<uint8_t>(regiment.y), static_cast<uint8_t>(data.current_side));
  }
  AnnounceOrder(game, prompts, buffer, person, kind);
  opeu_set_unit_order(game, unit, { .target = target, .cell = cell, .save_current = false, .code = kind });
  opeu_daily_context(game, static_cast<uint8_t>(data.current_side));
  opeu_delay_units(game, static_cast<uint8_t>(ORDER_DELAY));
  done();
}
