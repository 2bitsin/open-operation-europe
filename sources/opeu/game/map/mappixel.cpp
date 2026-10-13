// Map cells to screen pixels (x counts 8-pixel byte columns), the side recolouring of unit sprites, and which units the
// map shows.
#include "map/mappixel.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/recindex.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/stats.hpp"

#include <ranges>

using namespace opeu::game;

namespace { }

// The overview's pixel position of a cell.
auto OverviewPixel(Pixel cell) -> Pixel {
  return { static_cast<uint16_t>(static_cast<int16_t>(cell.x) + 2), static_cast<uint16_t>(static_cast<int16_t>(cell.y) * 8 + 68) };
}

// The tactical view's pixel position of a cell.
auto ViewPixel(DataSegment const& data, Pixel cell) -> Pixel {
  return { static_cast<uint16_t>((cell.x - static_cast<int16_t>(data.viewport_x)) * 3 + 4), static_cast<uint16_t>((cell.y - static_cast<int16_t>(data.viewport_y)) * 24 + 84) };
}

// The service boundary (#13 removes it): x and y are DS offsets of the cell's coordinate words, converted in place.
void opeu_overview_pixel(Game& game, uint16_t x, uint16_t y) {
  auto const pixel{ OverviewPixel({ Word(game, x), Word(game, y) }) };
  SetWord(game, x, pixel.x);
  SetWord(game, y, pixel.y);
}

void opeu_view_pixel(Game& game, uint16_t x, uint16_t y) {
  auto const pixel{ ViewPixel(Data(game), { Word(game, x), Word(game, y) }) };
  SetWord(game, x, pixel.x);
  SetWord(game, y, pixel.y);
}

// Side 0's sprites swap the first two bytes of every 3-byte group of the DS buffer at sprite.
void opeu_sprite_recolor(Game& game, uint16_t sprite, uint16_t size, uint8_t side) {
  if (side != 0) return;
  for (auto const group : std::views::iota(0, (size + 2) / 3)) {
    auto const first{ static_cast<uint16_t>(sprite + group * 3) };
    auto const second{ static_cast<uint16_t>(first + 1) };
    std::swap(Byte(game, first), Byte(game, second));
  }
}

// A unit of the side the map shows is visible when it can be selected, an enemy unit only when it has been spotted.
// Nothing is visible while the map shows no side.
uint16_t opeu_unit_visible(Game& game, uint16_t unit) {
  auto const& data{ Data(game) };
  uint8_t     side;
  if (data.overview_side != NO_SIDE)
    side = data.overview_side;
  else if (data.view_side != NO_SIDE)
    side = data.view_side;
  else
    return 0;
  if (opeu_unit_side(unit) == side) return opeu_unit_selection_eligible(game, unit);
  return opeu_unit_special(game, unit);
}
