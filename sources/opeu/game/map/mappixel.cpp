// Map cells to screen pixels (x counts 8-pixel byte columns), the side recolouring of unit sprites, and which units the
// map shows.
#include "types.hpp"
#include "map/mappixel.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/recindex.hpp"
#include "map/support.hpp"
#include "units/stats.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t NO_SIDE{ 255 };
}

// x and y: DS offsets of the cell's coordinate words, converted in place to the overview's pixel position.
void opeu_overview_pixel(uint8_t* ds, uint16_t x, uint16_t y) {
  SetWord(ds, x, static_cast<uint16_t>(static_cast<int16_t>(Word(ds, x)) + 2));
  SetWord(ds, y,
          static_cast<uint16_t>(static_cast<int16_t>(Word(ds, y)) * 8 + 68));
}

// x and y: DS offsets of the cell's coordinate words, converted in place to the tactical view's pixel position.
void opeu_view_pixel(uint8_t* ds, uint16_t x, uint16_t y) {
  auto const& data{ Data(ds) };
  SetWord(ds, x,
          static_cast<uint16_t>(
              (Word(ds, x) - static_cast<int16_t>(data.viewport_x)) * 3 + 4));
  SetWord(ds, y,
          static_cast<uint16_t>(
              (Word(ds, y) - static_cast<int16_t>(data.viewport_y)) * 24 + 84));
}

// Side 0's sprites swap the first two bytes of every 3-byte group of the DS buffer at sprite.
void opeu_sprite_recolor(uint8_t* ds, uint16_t sprite, uint16_t size,
                         uint8_t side) {
  if (side != 0)
    return;
  for (uint16_t at{ 0 }; at < size; at += 3) {
    auto const first{ static_cast<uint16_t>(sprite + at) };
    auto const second{ static_cast<uint16_t>(first + 1) };
    std::swap(ds[first], ds[second]);
  }
}

// A unit of the side the map shows is visible when it can be selected, an enemy unit only when it has been spotted.
// Nothing is visible while the map shows no side.
uint16_t opeu_unit_visible(uint8_t* ds, uint16_t unit) {
  auto const& data{ Data(ds) };
  uint8_t     side;
  if (data.overview_side != NO_SIDE)
    side = data.overview_side;
  else if (data.view_side != NO_SIDE)
    side = data.view_side;
  else
    return 0;
  if (opeu_unit_side(unit) == side)
    return opeu_unit_selection_eligible(ds, unit);
  return opeu_unit_special(ds, unit);
}
