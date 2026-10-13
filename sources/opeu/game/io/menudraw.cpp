// Button menus (common/menu.hpp): drawing and mouse selection. Buttons that ask for it have the graphics they cover
// saved into the bitmap buffer (at the running offset menu_save_offset) before drawing and restored afterwards in
// reverse order. Selection returns the item's selection id, or 0xffff for a right-button cancel when allowed.
#include "io/menudraw.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/menu.hpp"
#include "core/textout.hpp"
#include "io/input-call.hpp"
#include "io/mouse.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>

using namespace opeu::game;

namespace {

// The button tiles (data/fg35c0 opeu_menu_button_tiles): far segment and the offset of each 96-byte tile.
constexpr std::uint16_t TILES_SEGMENT{ 0x35c0 };
constexpr std::uint16_t TILE_BYTES{ 0x60 };
enum Tile : std::uint16_t {
  LEFT,
  MIDDLE,
  RIGHT,
  LEFT_LIT,
  MIDDLE_LIT,
  RIGHT_LIT,
};

constexpr std::uint16_t BUTTON_HEIGHT{ 24 };
constexpr std::uint16_t SAVED_BYTES_PER_COLUMN{ 0x60 };
constexpr std::uint16_t NOT_CHOSEN{ 0x8000 };
constexpr auto          MAX_ITEMS{ 30u };

auto ItemAt(Game& game, std::uint16_t near) -> MenuItem& {
  return Record<MenuItem>(Data(game), near);
}

// The bytes a saved button covers: its width plus a column either side.
auto SavedBytes(MenuItem const& item) -> std::uint16_t {
  return static_cast<std::uint16_t>((item.width + 2) * SAVED_BYTES_PER_COLUMN);
}
} // namespace

// Prints a label at (x, y) in colour attribute (its low byte), keeping the text context.
void opeu_menu_text(Game& game, std::uint16_t x, std::uint16_t y, std::uint16_t attribute, std::uint16_t label) {
  auto& data{ Data(game) };
  opeu_context_save(game, static_cast<TextHost*>(port_textout_host()));
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));
  opeu_cursor_print(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(Near(data, data.menu_format)), static_cast<std::uint16_t>(static_cast<std::uint8_t>(attribute) + '0'),
                                                        static_cast<std::uint16_t>(label) });
  opeu_context_restore(game, static_cast<TextHost*>(port_textout_host()));
}

// Tiles columns x..x+width at pixel column*8-4, then the label 3 (plain) or 4 (lit) pixels lower.
void opeu_menu_draw_item(Game& game, std::uint16_t near, int lit) {
  auto&         data{ Data(game) };
  auto const&   item{ ItemAt(game, near) };
  std::uint16_t column{ item.x };
  auto const    last{ static_cast<std::uint16_t>(column + item.width) };
  while (column <= last) {
    Tile tile{ MIDDLE };
    if (item.x == column)
      tile = LEFT;
    else if (last == column)
      tile = RIGHT;
    if (lit) tile = static_cast<Tile>(tile + LEFT_LIT);
    data.stream_ptr[0] = static_cast<std::uint16_t>(tile * TILE_BYTES);
    data.stream_ptr[1] = TILES_SEGMENT;
    koei_2e57(static_cast<std::uint16_t>((column << 3) - 4), item.y, 1, BUTTON_HEIGHT, port_far(data.stream_ptr[1], data.stream_ptr[0]));
    column++;
  }
  opeu_menu_text(game, item.x, static_cast<std::uint16_t>((lit ? 4 : 3) + item.y), item.attribute, item.label);
}

// Draws every item, saving the graphics under the SAVE_BEHIND items first.
void opeu_menu_draw(Game& game, std::uint16_t near) {
  auto& data{ Data(game) };
  data.menu_save_offset = 0;
  for (; ItemAt(game, near).label; near = NextMenuItem(near)) {
    auto const& item{ ItemAt(game, near) };
    if (item.selection & SAVE_BEHIND) {
      opeu_bitmap_save(item.x - 1, item.y, item.width + 2, BUTTON_HEIGHT, port_far(data.bitmap_buffer, data.menu_save_offset));
      data.menu_save_offset += SavedBytes(item);
    }
    opeu_menu_draw_item(game, near, 0);
  }
}

// Recurses through the terminator first, so the saved graphics (the terminator's included, when its selection asks
// for it) are restored in reverse order.
void opeu_menu_restore(Game& game, std::uint16_t near) {
  auto& data{ Data(game) };
  if (ItemAt(game, near).label) opeu_menu_restore(game, NextMenuItem(near));
  auto const& item{ ItemAt(game, near) };
  if (item.selection & SAVE_BEHIND) {
    data.menu_save_offset -= SavedBytes(item);
    opeu_bitmap_restore(item.x - 1, item.y, item.width + 2, BUTTON_HEIGHT, 0, port_far(data.bitmap_buffer, data.menu_save_offset));
  }
}

// Lights the item under the mouse (x*8..(x+width)*8-1, y..y+23); a left press chooses it, a right press cancels when
// allowed. One more pass after the choice puts out every light.
auto opeu_menu_track(Game& game, HostService service, std::uint16_t items, int cancel) -> std::uint16_t {
  std::array<int, MAX_ITEMS> lit{};
  opeu_input_reset(game, service);
  std::uint16_t chosen{ NOT_CHOSEN };
  opeu_delay_units(game, static_cast<std::uint8_t>(1));
  opeu_mouse_show(game, service);
  std::uint16_t passes{ 0 };
  do {
    std::uint16_t index{ 0 };
    for (std::uint16_t near{ items }; ItemAt(game, near).label; near = NextMenuItem(near), index++) {
      auto const&   item{ ItemAt(game, near) };
      std::uint16_t left   = item.x << 3;
      std::uint16_t top    = item.y;
      std::uint16_t right  = left + (item.width << 3) - 1;
      std::uint16_t bottom = top + 23;
      auto const [x, y]{ MousePosition(game, service) };
      if (!passes && left <= x && right >= x && top <= y && bottom >= y) {
        if (!lit[index]) {
          opeu_mouse_hide(game, service);
          opeu_menu_draw_item(game, near, lit[index] = 1);
          opeu_mouse_show(game, service);
        }
        if (opeu_mouse_left(game, service)) chosen = item.selection & SELECTION_MASK;
      } else if (lit[index]) {
        opeu_mouse_hide(game, service);
        opeu_menu_draw_item(game, near, lit[index] = 0);
        opeu_mouse_show(game, service);
      }
      if (cancel && opeu_mouse_right(game, service)) chosen = MENU_CANCELLED;
    }
    if (chosen != NOT_CHOSEN) passes++;
  } while (passes < 2);
  opeu_mouse_hide(game, service);
  return chosen;
}

auto opeu_menu_select(Game& game, HostService service, std::uint16_t items, int cancel) -> std::uint16_t {
  opeu_menu_draw(game, items);
  auto const chosen{ opeu_menu_track(game, service, items, cancel) };
  opeu_menu_restore(game, items);
  return chosen;
}

auto svc_menudraw(Game& game, std::uint16_t call, std::uint16_t* words) -> std::uint16_t {
  switch (static_cast<InputCall>(call)) {
  case InputCall::MOUSE_INT86:    return svc_mouse(game, call, words);
  case InputCall::TEXT_SAVE:      opeu_context_save(game, static_cast<TextHost*>(port_textout_host())); break;
  case InputCall::TEXT_POSITION:  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::int16_t>(words[0]), static_cast<std::int16_t>(words[1])); break;
  case InputCall::TEXT_PRINT:     opeu_cursor_print(game, { words, 3 }); break;
  case InputCall::TEXT_RESTORE:   opeu_context_restore(game, static_cast<TextHost*>(port_textout_host())); break;
  case InputCall::TILE_BLIT:      koei_2e57(words[0], words[1], words[2], words[3], port_far(words[4], words[5])); break;
  case InputCall::BITMAP_SAVE:    opeu_bitmap_save(words[0], words[1], words[2], words[3], port_far(words[4], words[5])); break;
  case InputCall::BITMAP_RESTORE: opeu_bitmap_restore(words[0], words[1], words[2], words[3], words[4], port_far(words[5], words[6])); break;
  case InputCall::DELAY_UNITS:    opeu_delay_units(game, static_cast<std::uint8_t>(words[0])); break;
  default:                        port_unhandled("menudraw", call, 7);
  }
  return 0;
}
