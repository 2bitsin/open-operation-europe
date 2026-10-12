// The supply-allocation panel: the keypad drawn on it, mouse and keyboard input, the supply table, the names in its
// title and the percentages drawn as digit glyphs.
#include "core/suppanel.hpp"
#include "core/strings.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/textout.hpp"
#include "glue.hpp"
#include "io/keyboard.hpp"
#include "io/mouse.hpp"
#include "io/waitkey.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>

using namespace opeu::game;

namespace {

using enum SupplyService;

// The keypad: three key columns and three key rows, in pixels from the panel's left edge and top.
constexpr std::array<std::uint16_t, 4> KEY_COLUMNS{ 0x48, 0x68, 0x98, 0xb8 };
constexpr std::array<std::uint16_t, 4> KEY_ROWS{ 0x23, 0x3f, 0x5b, 0x77 };
constexpr std::uint8_t                 KEYS[3][3]{
  { '7', '4', '1' },
  { '8', '5', '2' },
  { '9', '6', '6' }
};
constexpr std::uint8_t NO_KEY{ 0xff };
constexpr std::uint8_t ENTER{ 13 };

// The panel's background, saved in the number dialog's image segment.
constexpr std::uint16_t PANEL_WIDTH{ 0x20 }; // in 8-pixel columns
constexpr std::uint16_t PANEL_HEIGHT{ 0x78 };
constexpr std::uint16_t PANEL_IMAGE_AT{ 0x1f00 };

constexpr std::int16_t NAME_LIMIT{ 10 }; // characters of a name in the title

constexpr std::uint16_t GLYPH_SIZE{ 6 }; // digit glyphs are 6 x 6 pixels, 6 bytes each
constexpr std::uint16_t BLANK_GLYPH{ 10 };
constexpr std::uint16_t GLYPH_COLOUR{ 7 };

auto MousePosition(std::uint8_t* ds, HostEvent event) -> std::array<std::uint16_t, 2> {
  std::array<std::uint16_t, 2> position{};
  event(ds, std::to_underlying(MOUSE_POSITION), position.data(), 0);
  return position;
}
} // namespace

// The key under a point of the keypad, NO_KEY off it.
uint8_t opeu_supply_keypad_hit(uint8_t ds[65536], uint16_t x, uint16_t y) {
  auto const&  data{ Data(ds) };
  auto const   left{ static_cast<std::uint16_t>(data.supply_panel_x << 3) };
  auto const   top{ data.supply_panel_y };
  std::uint8_t key{ NO_KEY };
  for (auto column = 0u; column < 3; ++column) {
    if (static_cast<std::uint16_t>(left + KEY_COLUMNS[column]) > x
        || static_cast<std::uint16_t>(left + KEY_COLUMNS[column + 1]) <= x)
      continue;
    for (auto row = 0u; row < 3; ++row)
      if (static_cast<std::uint16_t>(top + KEY_ROWS[row]) <= y
          && static_cast<std::uint16_t>(top + KEY_ROWS[row + 1]) > y)
        key = KEYS[column][row];
  }
  return key;
}

// With the mouse: the right button is Enter and a left click on the keys 8, 5 or 2 selects row 0, 1 or 2 (stored at
// the near pointer row, NO_KEY returned). With the keyboard: the key.
uint8_t opeu_supply_input(uint8_t ds[65536], HostEvent event, uint16_t row) {
  auto& data{ Data(ds) };
  if (!data.input_mode) return static_cast<std::uint8_t>(Call(ds, event, KEY));
  Call(ds, event, MOUSE_SHOW);
  Call(ds, event, INPUT_DELAY, 3);
  std::uint8_t key;
  for (;;) {
    auto const [x, y]{ MousePosition(ds, event) };
    if (Call(ds, event, RIGHT_DOWN)) {
      key = ENTER;
      break;
    }
    if (Call(ds, event, LEFT_DOWN)) {
      key = opeu_supply_keypad_hit(ds, x, y);
      for (std::uint8_t selected = 0; selected < 3; ++selected)
        if (key == KEYS[1][selected]) {
          Record<std::uint8_t>(data, row) = selected;
          key                             = NO_KEY;
        }
      break;
    }
  }
  Call(ds, event, MOUSE_HIDE);
  return key;
}

void opeu_supply_set(uint8_t ds[65536], uint8_t side, uint8_t row, uint8_t column, uint16_t value) {
  Data(ds).supply_table[side][row][column] = value;
}

uint16_t opeu_supply_get(const uint8_t ds[65536], uint8_t side, uint8_t row, uint8_t column) {
  return Data(ds).supply_table[side][row][column];
}

// Restores the panel's background and centres the two names, at most ten characters each, in its title.
void opeu_supply_draw(uint8_t ds[65536], HostEvent event, uint16_t first, uint16_t second) {
  auto const& data{ Data(ds) };
  Call(ds, event, RESTORE, data.supply_panel_x, data.supply_panel_y, PANEL_WIDTH, PANEL_HEIGHT, 0,
       data.numdlg_image_segment, PANEL_IMAGE_AT);
  auto const indent{ [&](std::uint16_t name) {
    auto const length{ std::min(static_cast<std::int16_t>(opeu_string_length(ds, name)), NAME_LIMIT) };
    return static_cast<std::int16_t>(NAME_LIMIT - length) / 2;
  } };
  Call(ds, event, POSITION, indent(first) + data.supply_panel_x + 2, data.supply_panel_y + 14);
  Call(ds, event, PRINT, offsetof(DataSegment, supply_name_format), first);
  Call(ds, event, POSITION, indent(second) + data.supply_panel_x + 20, data.supply_panel_y + 14);
  Call(ds, event, PRINT, offsetof(DataSegment, supply_name_text), second);
}

// Draws value right-aligned leftwards from x as digit glyphs over blanks; zero draws one digit.
void opeu_supply_digits(uint8_t ds[65536], HostEvent event, uint16_t x, uint16_t y, uint16_t value) {
  auto const glyphs{ Near(Data(ds), Data(ds).digit_glyphs) };
  for (;; x -= GLYPH_SIZE) {
    Call(ds, event, GLYPH, x, y, GLYPH_SIZE, GLYPH_SIZE, 0, glyphs + GLYPH_SIZE * BLANK_GLYPH);
    Call(ds, event, GLYPH, x, y, GLYPH_SIZE, GLYPH_SIZE, GLYPH_COLOUR, glyphs + GLYPH_SIZE * (value % 10));
    if (!(value /= 10)) break;
  }
}

uint16_t svc_suppanel(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r{ 0 };
  switch (static_cast<SupplyService>(id)) {
  case MOUSE_SHOW:     opeu_mouse_show(ds, svc_mouse); break;
  case INPUT_DELAY:    opeu_delay_ticks(ds, svc_waitkey, Arg(args, count, 0)); break;
  case MOUSE_POSITION: {
    // The mouse routine writes x and y to two words on the DS stack; they go back to the caller's words.
    auto const at{ static_cast<std::uint16_t>(port_stack_enter(4) - 4) };
    auto*      out{ const_cast<std::uint16_t*>(args) };
    opeu_mouse_position(ds, svc_mouse, at, static_cast<std::uint16_t>(at + 2));
    out[0] = port_ds_word(at);
    out[1] = port_ds_word(static_cast<std::uint16_t>(at + 2));
    port_stack_leave(4);
    break;
  }
  case RIGHT_DOWN: r = static_cast<std::uint16_t>(opeu_mouse_right(ds, svc_mouse)); break;
  case LEFT_DOWN:  r = static_cast<std::uint16_t>(opeu_mouse_left(ds, svc_mouse)); break;
  case MOUSE_HIDE: opeu_mouse_hide(ds, svc_mouse); break;
  case KEY:        r = opeu_keyboard_key(ds, svc_keyboard); break;
  case RESTORE:
    opeu_bitmap_restore(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3),
                        Arg(args, count, 4), port_far(Arg(args, count, 5), Arg(args, count, 6)));
    break;
  case POSITION:
    opeu_army_position(ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1));
    break;
  case PRINT: b2_cursor_print(ds, args, count); break;
  case GLYPH:
    opeu_vga_glyph(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3),
                   Arg(args, count, 4), PORT_DS(Arg(args, count, 5)));
    break;
  default: port_unhandled("suppanel", id, count);
  }
  return r;
}
