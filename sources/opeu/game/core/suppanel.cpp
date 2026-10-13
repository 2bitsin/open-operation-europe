// The supply-allocation panel: the keypad drawn on it, mouse and keyboard input, the supply table, the names in its
// title and the percentages drawn as digit glyphs.
#include "core/suppanel.hpp"
#include "core/strings.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/exchange-panel.hpp"
#include "common/game.hpp"
#include "common/keys.hpp"
#include "common/memory.hpp"
#include "core/textout.hpp"
#include "io/keyboard.hpp"
#include "io/mouse.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>

using namespace opeu::game;

namespace {

// The keys of the keypad's columns, top to bottom (the original's bottom right key is a second 6).
constexpr std::uint8_t KEYS[3][3]{
  { '7', '4', '1' },
  { '8', '5', '2' },
  { '9', '6', '6' }
};
} // namespace

// The key under a point of the keypad, NO_KEY off it.
uint8_t opeu_supply_keypad_hit(Game& game, uint16_t x, uint16_t y) {
  auto const&  data{ Data(game) };
  auto const   left{ static_cast<std::uint16_t>(data.supply_panel_x << 3) };
  auto const   top{ data.supply_panel_y };
  std::uint8_t key{ NO_KEY };
  for (auto column = 0u; column < 3; ++column) {
    if (static_cast<std::uint16_t>(left + KEY_COLUMNS[column]) > x || static_cast<std::uint16_t>(left + KEY_COLUMNS[column + 1]) <= x) continue;
    for (auto row = 0u; row < 3; ++row)
      if (static_cast<std::uint16_t>(top + KEY_ROWS[row]) <= y && static_cast<std::uint16_t>(top + KEY_ROWS[row + 1]) > y) key = KEYS[column][row];
  }
  return key;
}

// With the mouse: the right button is Enter and a left click on the keys 8, 5 or 2 selects row 0, 1 or 2 (stored at
// the near pointer row, NO_KEY returned). With the keyboard: the key.
uint8_t opeu_supply_input(Game& game, uint16_t row) {
  auto& data{ Data(game) };
  if (!data.input_mode) return opeu_keyboard_key(game);
  opeu_mouse_show(game);
  opeu_delay_ticks(game, 3);
  std::uint8_t key;
  for (;;) {
    auto const [x, y]{ MousePosition(game) };
    if (static_cast<std::uint16_t>(opeu_mouse_right(game))) {
      key = KEY_ENTER;
      break;
    }
    if (static_cast<std::uint16_t>(opeu_mouse_left(game))) {
      key = opeu_supply_keypad_hit(game, x, y);
      for (std::uint8_t selected = 0; selected < 3; ++selected)
        if (key == KEYS[1][selected]) {
          Record<std::uint8_t>(data, row) = selected;
          key                             = NO_KEY;
        }
      break;
    }
  }
  opeu_mouse_hide(game);
  return key;
}

void opeu_supply_set(Game& game, uint8_t side, uint8_t row, uint8_t column, uint16_t value) {
  Data(game).supply_table[side][row][column] = value;
}

uint16_t opeu_supply_get(Game const& game, uint8_t side, uint8_t row, uint8_t column) {
  return Data(game).supply_table[side][row][column];
}

// Restores the panel's background and centres the two names, at most ten characters each, in its title.
void opeu_supply_draw(Game& game, uint16_t first, uint16_t second) {
  auto const& data{ Data(game) };
  opeu_bitmap_restore(data.supply_panel_x, data.supply_panel_y, PANEL_WIDTH, PANEL_HEIGHT, 0, port_far(data.numdlg_image_segment, PANEL_IMAGE_AT));
  auto const indent{ [&](std::uint16_t name) {
    auto const length{ std::min(static_cast<std::int16_t>(opeu_string_length(game, name)), NAME_LIMIT) };
    return static_cast<std::int16_t>(NAME_LIMIT - length) / 2;
  } };
  opeu_army_position(game, indent(first) + data.supply_panel_x + 2, data.supply_panel_y + 14);
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(offsetof(DataSegment, supply_name_format)), first });
  opeu_army_position(game, indent(second) + data.supply_panel_x + 20, data.supply_panel_y + 14);
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(offsetof(DataSegment, supply_name_text)), second });
}

// Draws value right-aligned leftwards from x as digit glyphs over blanks; zero draws one digit.
void opeu_supply_digits(Game& game, uint16_t x, uint16_t y, uint16_t value) {
  DrawDigits(game, x, y, value);
}
