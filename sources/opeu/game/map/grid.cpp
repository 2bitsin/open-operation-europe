// The terrain of strategic map cells: a bridge is built or destroyed by rewriting the cell's terrain, keeping its
// occupant and mark bits.
#include "map/grid.hpp"
#include "common/game.hpp"
#include "common/map-grid.hpp"
#include "types.hpp"

using namespace opeu::game;

namespace {
void SetTerrain(Game& game, uint8_t x, uint8_t y, Terrain terrain) {
  auto& cell{ MapCellAt(Data(game), x, y) };
  cell = static_cast<uint8_t>((cell & ~Bits(MapCell::TERRAIN_MASK)) | Bits(terrain));
}
} // namespace

void opeu_map_mark_10(Game& game, uint8_t x, uint8_t y) {
  SetTerrain(game, x, y, Terrain::BRIDGE);
}

void opeu_map_mark_5(Game& game, uint8_t x, uint8_t y) {
  SetTerrain(game, x, y, Terrain::RIVER);
}

uint8_t opeu_map_code(Game const& game, uint8_t x, uint8_t y) {
  return MapCellAt(Data(game), x, y) & Bits(MapCell::TERRAIN_MASK);
}
