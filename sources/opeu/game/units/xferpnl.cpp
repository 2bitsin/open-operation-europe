// The supply-transfer panel of the supply orders: the keypad drawn on it, mouse and keyboard input, the transfer
// table (per side and resource: the amount, the capacity and the computer's target), the names in its title, the
// rows with their fill percentages and the computer's keys. A twin of the supply-allocation panel (core/suppanel).
#include "units/xferpnl.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/exchange-panel.hpp"
#include "common/keys.hpp"
#include "common/memory.hpp"
#include "core/strings.hpp"
#include "types.hpp"
#include "units/ratio.hpp"
#include "units/transfer-service.hpp"

#include "common/game.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/recindex.hpp"
#include "core/textout.hpp"
#include "io/keyboard.hpp"
#include "io/mouse.hpp"
#include "io/sound.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "map/movetgt.hpp"
#include "map/support.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/aggpct.hpp"
#include "units/capacity.hpp"
#include "units/orders.hpp"
#include "units/resagg.hpp"
#include "units/stats.hpp"
#include "units/unitname.hpp"
#include "units/value.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <utility>

using namespace opeu::game;

namespace {

// The keys of the keypad's columns, top to bottom.
constexpr std::array<std::array<std::uint8_t, 3>, 3> KEYS{
  {
   { '7', '4', '1' },
   { '8', '5', '2' },
   { '9', '6', '3' },
   }
};

constexpr std::uint16_t BOX_WIDTH{ 3 }; // the percentage's background, in 8-pixel columns
constexpr std::uint16_t BOX_HEIGHT{ 6 };

// The computer's direction latch: which way it last moved supplies, so that it never moves them back.
constexpr std::uint8_t TOWARD_LEFT{ '<' };
constexpr std::uint8_t TOWARD_RIGHT{ '>' };
constexpr std::uint8_t UNMOVED{ '-' };
} // namespace

// The key under a point of the keypad, NO_KEY off it.
uint8_t opeu_transfer_keypad_hit(Game& game, uint16_t x, uint16_t y) {
  auto const& data{ Data(game) };
  auto const  left{ static_cast<std::uint16_t>(data.transfer_panel_x << 3) };
  auto const  top{ data.transfer_panel_y };
  // The band [edges[i], edges[i + 1]) of the panel holding the point, offset by `origin`.
  auto const band{ [](std::array<std::uint16_t, 4> const& edges, std::uint16_t origin, std::uint16_t point) {
    auto const below{ [&](std::uint16_t edge) { return static_cast<std::uint16_t>(origin + edge) <= point; } };
    return std::ranges::find_if_not(edges, below) - edges.begin() - 1;
  } };
  auto const column{ band(KEY_COLUMNS, left, x) };
  auto const row{ band(KEY_ROWS, top, y) };
  if (column < 0 || column > 2 || row < 0 || row > 2) return NO_KEY;
  return KEYS[static_cast<std::size_t>(column)][static_cast<std::size_t>(row)];
}

// With the mouse: the right button is Enter and a left click on the keys 8, 5 or 2 selects row 0, 1 or 2 (stored at
// the row, NO_KEY returned). With the keyboard: the key.
auto opeu_transfer_input(Game& game) -> PanelInput {
  auto& data{ Data(game) };
  if (!data.input_mode) return { static_cast<std::uint8_t>(opeu_keyboard_key(game)), std::nullopt };
  opeu_mouse_show(game);
  opeu_delay_ticks(game, 3);
  std::uint8_t                key{};
  std::optional<std::uint8_t> row;
  for (;;) {
    auto const [x, y]{ MousePosition(game) };
    if (static_cast<uint16_t>(opeu_mouse_right(game))) {
      key = KEY_ENTER;
      break;
    }
    if (static_cast<uint16_t>(opeu_mouse_left(game))) {
      key = opeu_transfer_keypad_hit(game, x, y);
      if (auto const selected{ std::ranges::find(KEYS[1], key) }; selected != KEYS[1].end()) {
        row = static_cast<std::uint8_t>(selected - KEYS[1].begin());
        key = NO_KEY;
      }
      break;
    }
  }
  opeu_mouse_hide(game);
  return { key, row };
}

// For a player's side: titles the panel, restores its background and centres the two names, at most ten characters
// each.
void opeu_transfer_draw(Game& game, Text first, Text second) {
  auto const& data{ Data(game) };
  if (!SidePlayed(data, data.current_side)) return;
  opeu_army_title(game, offsetof(DataSegment, transfer_title));
  game.Machine().Display().DrawColumns({ data.transfer_panel_x, data.transfer_panel_y, PANEL_WIDTH, PANEL_HEIGHT }, { 0, FarBytes(game, data.numdlg_image_segment, PANEL_IMAGE_AT) });
  auto const indent{ [&](Text name) {
    auto const length{ std::min(static_cast<std::int16_t>(opeu_string_length(game, name)), NAME_LIMIT) };
    return static_cast<std::int16_t>(NAME_LIMIT - length) / 2;
  } };
  opeu_army_position(game, static_cast<int16_t>(indent(first) + data.transfer_panel_x + 2), static_cast<int16_t>(data.transfer_panel_y + 14));
  opeu_cursor_print(game, std::array<FormatArgument, 2>{ offsetof(DataSegment, transfer_name_format), first });
  opeu_army_position(game, static_cast<int16_t>(indent(second) + data.transfer_panel_x + 20), static_cast<int16_t>(data.transfer_panel_y + 14));
  opeu_cursor_print(game, std::array<FormatArgument, 2>{ offsetof(DataSegment, transfer_name_text), second });
}

// For a player's side, redraws a resource row: both amounts and both fill percentages.
void opeu_transfer_draw_row(Game& game, uint8_t row) {
  auto const& data{ Data(game) };
  if (!SidePlayed(data, data.current_side)) return;
  auto const& left{ data.transfer_table[0][row] };
  auto const& right{ data.transfer_table[1][row] };
  auto const  left_amount{ left[AMOUNT] };
  auto const  right_amount{ right[AMOUNT] };
  auto const  left_percent{ opeu_resource_percent(left_amount, left[CAPACITY]) };
  auto const  right_percent{ opeu_resource_percent(right_amount, right[CAPACITY]) };
  auto const  x{ data.transfer_panel_x };
  auto const  text_y{ row * PANEL_ROW_HEIGHT + data.transfer_panel_y + 0x24 };
  auto const  box_y{ static_cast<std::uint16_t>(row * PANEL_ROW_HEIGHT + data.transfer_panel_y + 0x37) };
  opeu_army_position(game, static_cast<int16_t>(x + 2), static_cast<int16_t>(text_y));
  opeu_cursor_print(game, std::array<FormatArgument, 2>{ offsetof(DataSegment, transfer_amount_format), left_amount });
  opeu_army_position(game, static_cast<int16_t>(x + 0x19), static_cast<int16_t>(text_y));
  opeu_cursor_print(game, std::array<FormatArgument, 2>{ offsetof(DataSegment, transfer_amount_text), right_amount });
  game.Machine().Display().Box({ static_cast<uint16_t>(x + 2), box_y, BOX_WIDTH, BOX_HEIGHT }, 0);
  DrawDigits(game, ScreenPoint{ static_cast<std::uint16_t>((x + 4) << 3), box_y }, left_percent);
  game.Machine().Display().Box({ static_cast<uint16_t>(x + 0x19), box_y, BOX_WIDTH, BOX_HEIGHT }, 0);
  DrawDigits(game, ScreenPoint{ static_cast<std::uint16_t>((x + 0x1b) << 3), box_y }, right_percent);
  opeu_cursor_print(game, std::array<FormatArgument, 1>{ offsetof(DataSegment, transfer_colour) });
}

// The computer's key for a resource row. Supplies move to the left side ('7', '4', '1') while its target exceeds its
// amount and the right side holds more than its target, or to the right side ('9', '6', '3') in the opposite case;
// the step compares the unsigned difference with 100 and 10 (so a move to the left is always '7'). The direction
// latch keeps the computer from moving supplies back. Otherwise '2' selects the next row, and Enter ends after the
// last one.
uint8_t opeu_transfer_auto_key(Game& game, uint8_t row) {
  auto&       data{ Data(game) };
  auto&       latch{ data.transfer_auto };
  auto const& left{ data.transfer_table[0][row] };
  auto const& right{ data.transfer_table[1][row] };
  auto const  left_amount{ left[AMOUNT] };
  auto const  right_amount{ right[AMOUNT] };
  auto const  left_target{ left[TARGET] };
  auto const  right_target{ right[TARGET] };
  if (latch != TOWARD_RIGHT && left_target > left_amount && right_target < right_amount) {
    latch = TOWARD_LEFT;
    auto const step{ static_cast<std::uint16_t>(right_target - right_amount) };
    return step >= 100 ? '7' : step >= 10 ? '4' : '1';
  }
  if (latch != TOWARD_LEFT && left_target < left_amount && right_target > right_amount) {
    latch = TOWARD_RIGHT;
    auto const step{ static_cast<std::uint16_t>(left_target - left_amount) };
    return step >= 100 ? '9' : step >= 10 ? '6' : '3';
  }
  if (row == RESOURCES - 1) return KEY_ENTER;
  latch = UNMOVED;
  return '2';
}
