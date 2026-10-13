// Placing a unit on the strategic map, removing it, and the side whose unit stands on a cell.
#include "map/occupy.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/map-grid.hpp"
#include "core/recindex.hpp"
#include "types.hpp"

using namespace opeu::game;

namespace { }

// Only a unit with elements is placed: it becomes ready and its side's bit is set in its cell.
void opeu_place_unit(Game& game, uint16_t unit) {
  auto& data{ Data(game) };
  auto& record{ Record<Unit>(data, unit) };
  if (!record.first) return;
  record.flags |= Bits(UnitFlag::READY);
  auto& cell{ MapCellAt(data, record.x, record.y) };
  cell &= ~Bits(MapCell::OCCUPANT_MASK);
  cell |= opeu_unit_side(unit) == 0 ? Bits(MapCell::SIDE_0) : Bits(MapCell::SIDE_1);
}

void opeu_remove_unit(Game& game, uint16_t unit) {
  auto& data{ Data(game) };
  auto& record{ Record<Unit>(data, unit) };
  record.flags                        &= ~Bits(UnitFlag::READY);
  MapCellAt(data, record.x, record.y) &= ~Bits(MapCell::OCCUPANT_MASK);
}

// 255 for an empty cell; side 0 wins when both occupant bits are set.
uint8_t opeu_cell_side(Game const& game, uint8_t x, uint8_t y) {
  auto const occupant{ MapCellAt(Data(game), x, y) & Bits(MapCell::OCCUPANT_MASK) };
  if (!occupant) return NO_SIDE;
  return (occupant & Bits(MapCell::SIDE_0)) == 0;
}
