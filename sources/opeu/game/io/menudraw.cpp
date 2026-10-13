// Button menus (common/menu.hpp): drawing and mouse selection. Buttons that ask for it have the graphics they cover
// saved into the bitmap buffer (at the running offset menu_save_offset) before drawing and restored afterwards in
// reverse order. Selection returns the item's selection id, or 0xffff for a right-button cancel when allowed.
#include "io/menudraw.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/menu.hpp"
#include "core/textout.hpp"
#include "io/mouse.hpp"
#include "io/waitkey.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <array>
#include <ranges>

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

// How many items a menu has before its terminator.
auto ItemCount(Game& game, std::uint16_t first) -> std::uint32_t {
  auto const labelled{ [&](std::uint32_t i) { return ItemAt(game, static_cast<std::uint16_t>(first + i * sizeof(MenuItem))).label != 0; } };
  auto const items{ std::views::iota(0u) | std::views::take_while(labelled) };
  return static_cast<std::uint32_t>(std::ranges::distance(items));
}

// The near pointers of `count` items from `first`.
auto ItemsOf(std::uint16_t first, std::uint32_t count) {
  return std::views::iota(0u, count) | std::views::transform([first](std::uint32_t i) { return static_cast<std::uint16_t>(first + i * sizeof(MenuItem)); });
}

// The bytes a saved button covers: its width plus a column either side.
auto SavedBytes(MenuItem const& item) -> std::uint16_t {
  return static_cast<std::uint16_t>((item.width + 2) * SAVED_BYTES_PER_COLUMN);
}

// Prints the item's label at pixel row y in the colour of its attribute's low byte, keeping the text context.
auto PrintLabel(Game& game, MenuItem const& item, std::uint16_t y) -> void {
  auto& data{ Data(game) };
  opeu_context_save(game);
  opeu_army_position(game, static_cast<std::int16_t>(item.x), static_cast<std::int16_t>(y));
  opeu_cursor_print(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(Near(data, data.menu_format)), static_cast<std::uint16_t>(static_cast<std::uint8_t>(item.attribute) + '0'),
                                                        static_cast<std::uint16_t>(item.label) });
  opeu_context_restore(game);
}

// The tile of a button column: the ends have their own.
auto TileAt(MenuItem const& item, std::uint32_t column, MenuLight light) -> Tile {
  auto const last{ static_cast<std::uint16_t>(item.x + item.width) };
  Tile       tile{ MIDDLE };
  if (item.x == column)
    tile = LEFT;
  else if (last == column)
    tile = RIGHT;
  return light == MenuLight::LIT ? static_cast<Tile>(tile + LEFT_LIT) : tile;
}

// Saves the graphics a SAVE_BEHIND item covers at the running offset.
auto SaveBehind(Game& game, MenuItem const& item) -> void {
  auto& data{ Data(game) };
  if (!(item.selection & SAVE_BEHIND)) return;
  game.Machine().Display().SaveColumns({ static_cast<uint16_t>(item.x - 1), item.y, static_cast<uint16_t>(item.width + 2), BUTTON_HEIGHT }, FarBytes(game, data.bitmap_buffer, data.menu_save_offset));
  data.menu_save_offset += SavedBytes(item);
}

auto RestoreBehind(Game& game, MenuItem const& item) -> void {
  auto& data{ Data(game) };
  if (!(item.selection & SAVE_BEHIND)) return;
  data.menu_save_offset -= SavedBytes(item);
  game.Machine().Display().DrawColumns({ static_cast<uint16_t>(item.x - 1), item.y, static_cast<uint16_t>(item.width + 2), BUTTON_HEIGHT },
                                       { 0, FarBytes(game, data.bitmap_buffer, data.menu_save_offset) });
}

// Whether the pointer is on the item: x*8..(x+width)*8-1, y..y+23.
auto Under(MenuItem const& item, ScreenPoint const& mouse) -> bool {
  std::uint16_t const left   = item.x << 3;
  std::uint16_t const top    = item.y;
  std::uint16_t const right  = left + (item.width << 3) - 1;
  std::uint16_t const bottom = top + 23;
  return left <= mouse.x && right >= mouse.x && top <= mouse.y && bottom >= mouse.y;
}

// Redraws an item with the mouse pointer hidden.
auto Relight(Game& game, std::uint16_t item, MenuLight light) -> void {
  opeu_mouse_hide(game);
  opeu_menu_draw_item(game, item, light);
  opeu_mouse_show(game);
}
} // namespace

// Tiles columns x..x+width at pixel column*8-4, then the label 3 (plain) or 4 (lit) pixels lower.
void opeu_menu_draw_item(Game& game, std::uint16_t near, MenuLight light) {
  auto&       data{ Data(game) };
  auto const& item{ ItemAt(game, near) };
  std::ranges::for_each(std::views::iota(std::uint32_t{ item.x }, item.x + item.width + 1u), [&](std::uint32_t column) {
    SetStream(data, TILES_SEGMENT, static_cast<std::uint16_t>(TileAt(item, column, light) * TILE_BYTES));
    game.Machine().Display().DrawShiftedColumns({ static_cast<std::uint16_t>((column << 3) - 4), item.y, 1, BUTTON_HEIGHT }, FarBytes(game, data.stream_ptr[1], data.stream_ptr[0]));
  });
  PrintLabel(game, item, static_cast<std::uint16_t>((light == MenuLight::LIT ? 4 : 3) + item.y));
}

// Draws every item, saving the graphics under the SAVE_BEHIND items first.
void opeu_menu_draw(Game& game, std::uint16_t first) {
  Data(game).menu_save_offset = 0;
  std::ranges::for_each(ItemsOf(first, ItemCount(game, first)), [&](std::uint16_t near) {
    SaveBehind(game, ItemAt(game, near));
    opeu_menu_draw_item(game, near, MenuLight::PLAIN);
  });
}

// Restores the saved graphics in reverse order, the terminator's first when its selection asks for it.
void opeu_menu_restore(Game& game, std::uint16_t first) {
  std::ranges::for_each(ItemsOf(first, ItemCount(game, first) + 1) | std::views::reverse, [&](std::uint16_t near) { RestoreBehind(game, ItemAt(game, near)); });
}

namespace {

// Lights the item under the mouse; a left press chooses it, a right press cancels when allowed. One more pass after
// the choice puts out every light.
auto Track(Game& game, std::uint16_t first, int cancel) -> std::uint16_t {
  std::array<MenuLight, MAX_ITEMS> lights{};
  auto const                       items{ ItemsOf(first, ItemCount(game, first)) };
  std::uint16_t                    chosen{ NOT_CHOSEN };
  std::uint16_t                    passes{ 0 };
  do {
    for (auto const [near, light] : std::views::zip(items, lights)) {
      auto const& item{ ItemAt(game, near) };
      auto const  mouse{ MousePosition(game) };
      if (!passes && Under(item, mouse)) {
        if (light != MenuLight::LIT) Relight(game, near, light = MenuLight::LIT);
        if (opeu_mouse_left(game)) chosen = item.selection & SELECTION_MASK;
      } else if (light == MenuLight::LIT)
        Relight(game, near, light = MenuLight::PLAIN);
      if (cancel && opeu_mouse_right(game)) chosen = MENU_CANCELLED;
    }
    if (chosen != NOT_CHOSEN) passes++;
  } while (passes < 2);
  return chosen;
}
} // namespace

auto opeu_menu_select(Game& game, std::uint16_t items, int cancel) -> std::uint16_t {
  opeu_menu_draw(game, items);
  opeu_input_reset(game);
  opeu_delay_units(game, 1);
  opeu_mouse_show(game);
  auto const chosen{ Track(game, items, cancel) };
  opeu_mouse_hide(game);
  opeu_menu_restore(game, items);
  return chosen;
}
