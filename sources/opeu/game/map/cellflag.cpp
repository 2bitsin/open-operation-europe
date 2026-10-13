// The sides' marks on strategic map cells and the influence the sides hold over them.
#include "map/cellflag.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/map-grid.hpp"
#include "types.hpp"

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

void opeu_mark_cell(Game& game, GridCell cell, uint8_t side) {
  MapCellAt(Data(game), cell.x, cell.y) |= side == 0 ? Bits(MapCell::MARK_0) : Bits(MapCell::MARK_1);
}

void opeu_unmark_cell(Game& game, GridCell cell, uint8_t side) {
  MapCellAt(Data(game), cell.x, cell.y) &= side == 0 ? ~Bits(MapCell::MARK_0) : ~Bits(MapCell::MARK_1);
}

// Side 255 asks whether either side marked the cell.
uint16_t opeu_cell_marked(Game& game, GridCell cell, uint8_t side) {
  return (MapCellAt(Data(game), cell.x, cell.y) & MarkOf(side)) != 0;
}

int16_t opeu_cell_influence(Game& game, GridCell cell) {
  return InfluenceAt(Data(game), cell.x, cell.y);
}

// The side whose influence holds the cell, 255 where neither does.
uint8_t opeu_cell_influence_side(Game& game, GridCell cell) {
  auto const influence{ opeu_cell_influence(game, cell) };
  if (influence > 0) return 0;
  if (influence < 0) return 1;
  return 255;
}
