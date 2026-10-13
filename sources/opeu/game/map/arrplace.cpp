// Picks an arrival cell for a side from the candidate grid that the request-cell scan fills. Each candidate after the
// first replaces the pick with probability 1/n, n being the number of candidates.
#include "map/arrplace.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "map/cellpred.hpp"
#include "map/reqcells.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t NONE{ 255 };
} // namespace

// x and y: DS offsets of the bytes that receive the cell; returns 0 when there is no candidate.
uint16_t opeu_arrival_locate(Game& game, uint16_t x, uint16_t y, uint8_t side) {
  auto const count{ opeu_request_cells(game, side, opeu_arrival_cell_allowed) };
  if (!count) return 0;
  auto const& data{ Data(game) };
  Byte(game, x) = NONE;
  for (uint8_t cell_y{ 0 }; cell_y < MAP_HEIGHT; ++cell_y)
    for (uint8_t cell_x{ 0 }; cell_x < MAP_WIDTH; ++cell_x)
      if (data.arrival_candidates[cell_y * MAP_WIDTH + cell_x] == 1 && (Byte(game, x) == NONE || RandomBelow(game, count) == 0)) {
        Byte(game, x) = cell_x;
        Byte(game, y) = cell_y;
      }
  return 1;
}
