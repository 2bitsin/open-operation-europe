// The sides' marks on strategic map cells and the influence the sides hold over them.
#include "types.hpp"
#include "map/cellflag.hpp"
#include "common/data-segment.hpp"
#include "common/map-grid.hpp"

using namespace opeu::game;

namespace {
// The mark bit of a side; any other side value stands for both marks.
[[nodiscard]] auto MarkOf(uint8_t side) -> uint8_t {
  switch (side) {
    case 0:  return Bits(MapCell::MARK_0);
    case 1:  return Bits(MapCell::MARK_1);
    default: return Bits(MapCell::MARK_MASK);
  }
}
} // namespace

void opeu_mark_cell(uint8_t* ds, uint8_t x, uint8_t y, uint8_t side) {
  MapCellAt(Data(ds), x, y) |=
      side == 0 ? Bits(MapCell::MARK_0) : Bits(MapCell::MARK_1);
}

void opeu_unmark_cell(uint8_t* ds, uint8_t x, uint8_t y, uint8_t side) {
  MapCellAt(Data(ds), x, y) &=
      side == 0 ? ~Bits(MapCell::MARK_0) : ~Bits(MapCell::MARK_1);
}

// Side 255 asks whether either side marked the cell.
uint16_t opeu_cell_marked(uint8_t* ds, uint8_t x, uint8_t y, uint8_t side) {
  return (MapCellAt(Data(ds), x, y) & MarkOf(side)) != 0;
}

int16_t opeu_cell_influence(uint8_t* ds, uint8_t x, uint8_t y) {
  return InfluenceAt(Data(ds), x, y);
}

// The side whose influence holds the cell, 255 where neither does.
uint8_t opeu_cell_influence_side(uint8_t* ds, uint8_t x, uint8_t y) {
  auto const influence{ opeu_cell_influence(ds, x, y) };
  if (influence > 0)
    return 0;
  if (influence < 0)
    return 1;
  return 255;
}
