// The terrain of strategic map cells: a bridge is built or destroyed by rewriting the cell's terrain, keeping its
// occupant and mark bits.
#include "types.hpp"
#include "map/grid.hpp"
#include "common/map-grid.hpp"

using namespace opeu::game;

namespace {
void SetTerrain(uint8_t* ds, uint8_t x, uint8_t y, Terrain terrain) {
  auto& cell{ MapCellAt(Data(ds), x, y) };
  cell = static_cast<uint8_t>((cell & ~Bits(MapCell::TERRAIN_MASK)) |
                              Bits(terrain));
}
} // namespace

void opeu_map_mark_10(uint8_t* ds, uint8_t x, uint8_t y) {
  SetTerrain(ds, x, y, Terrain::BRIDGE);
}

void opeu_map_mark_5(uint8_t* ds, uint8_t x, uint8_t y) {
  SetTerrain(ds, x, y, Terrain::RIVER);
}

uint8_t opeu_map_code(const uint8_t* ds, uint8_t x, uint8_t y) {
  return MapCellAt(Data(ds), x, y) & Bits(MapCell::TERRAIN_MASK);
}
