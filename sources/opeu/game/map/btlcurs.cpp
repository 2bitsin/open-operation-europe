// The player's battle cursor: whether a cell is open to an element, the cursor's drawing and steps, and the cell under
// a mouse pixel.
//
// The field is 14 x 14 hexes in 20 x 26-pixel columns. The mouse cell refines its column guess with the hex-shape bit
// mask (26 rows of 5 bytes, 8 pixels a byte) in its far segment.
#include "map/btlcurs.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btlcalc.hpp"
#include "map/btldraw.hpp"
#include "map/btllist.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "map/btlcmds.hpp"
#include "port.hpp"
#include "services.hpp"

#include <optional>
#include <utility>

using namespace opeu::game;

namespace {
constexpr int16_t LAST_CELL{ BATTLE_WIDTH - 1 };
constexpr int16_t NO_CELL{ BATTLE_WIDTH }; // the mouse cell off the field
constexpr int16_t HEX_WIDTH{ 20 };
constexpr int16_t HEX_HEIGHT{ 26 };
constexpr int16_t MASK_ROW_BYTES{ 5 };

constexpr uint16_t HEX_MASK_SEGMENT{ 0x352d }; // the far segment of the hex-shape masks

// A byte of the hex-shape mask.
[[nodiscard]] auto HexMask(int16_t row, int16_t column) -> uint8_t {
  return *port_far(HEX_MASK_SEGMENT, static_cast<uint16_t>(row * MASK_ROW_BYTES + column));
}

// A field cell guess in signed column and row, possibly off the field.
struct CellGuess {
  int16_t column;
  int16_t row;
};

// The cell under a pixel of the screen's right half, refined by the hex mask.
[[nodiscard]] auto RightHalfCell(int16_t x, int16_t y) -> CellGuess {
  auto       column{ static_cast<int16_t>((x - 320) / HEX_WIDTH) };
  auto const top{ static_cast<int16_t>((column + 2) * 13) };
  auto       row{ static_cast<int16_t>((y - top) / HEX_HEIGHT) };
  column = static_cast<int16_t>(column + row);
  auto const mask_row{ static_cast<int16_t>(x % 40 < HEX_WIDTH ? y % HEX_HEIGHT : (y + 13) % HEX_HEIGHT) };
  auto const mask_byte{ static_cast<int16_t>((x % HEX_WIDTH + 20) / 8) };
  if (!(HexMask(mask_row, mask_byte) & static_cast<uint8_t>(0x80 >> (x % HEX_WIDTH + 20) % 8))) {
    if (mask_row < 13)
      --row;
    else
      ++column;
  }
  return { column, row };
}

// The cell under a pixel of the screen's left half (from x = 40), refined by the hex mask.
[[nodiscard]] auto LeftHalfCell(int16_t x, int16_t y) -> CellGuess {
  auto       row{ static_cast<int16_t>((40 - x) / HEX_WIDTH + 13) };
  auto const top{ static_cast<int16_t>((row + 2) * 13) };
  auto       column{ static_cast<int16_t>((y - top) / HEX_HEIGHT) };
  row = static_cast<int16_t>(row + column);
  auto const mask_row{ static_cast<int16_t>(x % 40 >= HEX_WIDTH ? y % HEX_HEIGHT : (y + 13) % HEX_HEIGHT) };
  auto const mask_byte{ static_cast<int16_t>(x % HEX_WIDTH / 8) };
  if (!(HexMask(mask_row, mask_byte) & static_cast<uint8_t>(0x80 >> x % HEX_WIDTH % 8))) {
    if (mask_row < 13)
      --column;
    else
      ++row;
  }
  return { column, row };
}
} // namespace

auto opeu_battle_cell_open(Game& game, GridCell cell, uint16_t element) -> uint16_t {
  if (!opeu_battle_cell_occupied(game, cell.x, cell.y)) return 1;
  auto const other{ opeu_battle_element_at(game, static_cast<uint8_t>(cell.x), static_cast<uint8_t>(cell.y)) };
  return other && opeu_battle_element_side(game, element) != opeu_battle_element_side(game, other) && !opeu_battle_element_flag2(game, other);
}

void opeu_battle_cursor_draw(Game& game) {
  auto const& data{ Data(game) };
  opeu_battle_draw_cursor(game, data.battle_cursor_x, data.battle_cursor_y);
}

void opeu_battle_cursor_step(Game& game, int16_t dx, int16_t dy) {
  auto& data{ Data(game) };
  opeu_battle_cursor_draw(game);
  auto const x{ static_cast<int16_t>(data.battle_cursor_x + dx) };
  if (x >= 0 && x <= LAST_CELL) data.battle_cursor_x = static_cast<uint8_t>(x);
  auto const y{ static_cast<int16_t>(data.battle_cursor_y + dy) };
  if (y >= 0 && y <= LAST_CELL) data.battle_cursor_y = static_cast<uint8_t>(y);
  opeu_battle_cursor_draw(game);
}

auto opeu_battle_mouse_cell(ScreenPoint pixel) -> std::optional<GridCell> {
  auto const x{ static_cast<int16_t>(pixel.x) };
  auto const y{ static_cast<int16_t>(pixel.y) };
  if (x < 320 && x < 40) return std::nullopt;
  auto const guess{ x >= 320 ? RightHalfCell(x, y) : LeftHalfCell(x, y) };
  if (guess.column < 0 || guess.column >= NO_CELL || guess.row < 0 || guess.row >= NO_CELL) return std::nullopt;
  return GridCell{ static_cast<uint8_t>(guess.column), static_cast<uint8_t>(guess.row) };
}
