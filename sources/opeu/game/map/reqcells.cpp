// The request-cell scan: clears the candidate grid, then for each map border the side may enter from (its eight
// request_directions) runs rays in from that border, each ray stopping where the predicate says so, and counts the
// candidates the predicate marked. Rays that run toward coordinate 0 stop before it.
#include "map/reqcells.hpp"
#include "common/data-segment.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"

using namespace opeu::game;

namespace {
using Predicate = uint16_t (*)(Game&, uint8_t, uint8_t, uint8_t);
constexpr uint8_t LAST{ MAP_WIDTH - 1 };
constexpr uint8_t HALF{ MAP_WIDTH / 2 };

// Rays along x for the rows [y_begin, y_end), from x = 0 up or from x = 39 down, each until the predicate stops it.
void RowRays(Game& game, uint8_t side, Predicate visit, uint8_t y_begin, uint8_t y_end, bool reverse) {
  for (uint8_t y{ y_begin }; y < y_end; ++y)
    if (reverse) {
      for (uint8_t x{ LAST }; x > 0; --x)
        if (visit(game, x, y, side)) break;
    } else {
      for (uint8_t x{ 0 }; x < MAP_WIDTH; ++x)
        if (visit(game, x, y, side)) break;
    }
}

// Rays along y for the columns [x_begin, x_end).
void ColumnRays(Game& game, uint8_t side, Predicate visit, uint8_t x_begin, uint8_t x_end, bool reverse) {
  for (uint8_t x{ x_begin }; x < x_end; ++x)
    if (reverse) {
      for (uint8_t y{ LAST }; y > 0; --y)
        if (visit(game, x, y, side)) break;
    } else {
      for (uint8_t y{ 0 }; y < MAP_HEIGHT; ++y)
        if (visit(game, x, y, side)) break;
    }
}
} // namespace

uint16_t opeu_count_request_cells(Game& game) {
  uint16_t count{ 0 };
  for (auto const candidate : Data(game).arrival_candidates)
    if (candidate == 1) ++count;
  return count;
}

uint16_t opeu_request_cells(Game& game, uint8_t side, Predicate predicate) {
  auto& data{ Data(game) };
  data.arrival_candidates.fill(0);
  auto const enabled{ [&](unsigned border) { return data.request_directions[side * 8 + border] == 1; } };
  if (enabled(0)) {
    RowRays(game, side, predicate, HALF, MAP_HEIGHT, false);
    ColumnRays(game, side, predicate, 0, HALF, true);
  }
  if (enabled(1)) ColumnRays(game, side, predicate, 0, MAP_WIDTH, true);
  if (enabled(2)) {
    ColumnRays(game, side, predicate, HALF, MAP_WIDTH, true);
    RowRays(game, side, predicate, HALF, MAP_HEIGHT, true);
  }
  if (enabled(3)) RowRays(game, side, predicate, 0, MAP_HEIGHT, false);
  if (enabled(4)) RowRays(game, side, predicate, 0, MAP_HEIGHT, true);
  if (enabled(5)) {
    RowRays(game, side, predicate, 0, HALF, false);
    ColumnRays(game, side, predicate, 0, HALF, false);
  }
  if (enabled(6)) ColumnRays(game, side, predicate, 0, MAP_WIDTH, false);
  if (enabled(7)) {
    ColumnRays(game, side, predicate, HALF, MAP_WIDTH, false);
    RowRays(game, side, predicate, 0, HALF, true);
  }
  return opeu_count_request_cells(game);
}
