// Predicates of the border scans that pick request and arrival cells: each returns 0 to stop the scan ray (an
// impassable cell), 1 to go on, and marks an acceptable cell in arrival_candidates on the way.
#include "map/cellpred.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "map/cellflag.hpp"
#include "map/grid.hpp"
#include "map/occupy.hpp"
#include "types.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t ANY_SIDE{ 255 };

void MarkCandidate(Game& game, uint8_t x, uint8_t y) {
  Data(game).arrival_candidates[y * MAP_WIDTH + x] = 1;
}
} // namespace

// Accepts any passable cell in the side's influence.
uint16_t opeu_request_cell_allowed(Game& game, uint8_t x, uint8_t y, uint8_t side) {
  if (opeu_map_code(game, x, y) == Bits(Terrain::IMPASSABLE)) return 0;
  if (opeu_cell_influence_side(game, x, y) == side) MarkCandidate(game, x, y);
  return 1;
}

// Accepts an empty, unmarked cell in the side's influence that is neither a city nor a river.
uint16_t opeu_arrival_cell_allowed(Game& game, uint8_t x, uint8_t y, uint8_t side) {
  switch (static_cast<Terrain>(opeu_map_code(game, x, y))) {
  case Terrain::IMPASSABLE: return 0;
  case Terrain::CITY:
  case Terrain::RIVER:      break;
  default:
    if (opeu_cell_influence_side(game, x, y) == side && opeu_cell_side(game, x, y) == ANY_SIDE && !opeu_cell_marked(game, x, y, ANY_SIDE)) MarkCandidate(game, x, y);
    break;
  }
  return 1;
}
