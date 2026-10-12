// The request-cell scan: clears the candidate grid, then for each map border the side may enter from (its eight
// request_directions) runs rays in from that border, each ray stopping where the predicate says so, and counts the
// candidates the predicate marked. Rays that run toward coordinate 0 stop before it.
#include "map/reqcells.hpp"
#include "common/data-segment.hpp"
#include "types.hpp"

#include "port.hpp"

using namespace opeu::game;

namespace {
using Predicate = uint16_t (*)(uint8_t*, uint8_t, uint8_t, uint8_t);
constexpr uint8_t LAST{ MAP_WIDTH - 1 };
constexpr uint8_t HALF{ MAP_WIDTH / 2 };

// Rays along x for the rows [y_begin, y_end), from x = 0 up or from x = 39 down, each until the predicate stops it.
void RowRays(uint8_t* ds, uint8_t side, Predicate visit, uint8_t y_begin, uint8_t y_end, bool reverse) {
  for (uint8_t y{ y_begin }; y < y_end; ++y)
    if (reverse) {
      for (uint8_t x{ LAST }; x > 0; --x)
        if (visit(ds, x, y, side)) break;
    } else {
      for (uint8_t x{ 0 }; x < MAP_WIDTH; ++x)
        if (visit(ds, x, y, side)) break;
    }
}

// Rays along y for the columns [x_begin, x_end).
void ColumnRays(uint8_t* ds, uint8_t side, Predicate visit, uint8_t x_begin, uint8_t x_end, bool reverse) {
  for (uint8_t x{ x_begin }; x < x_end; ++x)
    if (reverse) {
      for (uint8_t y{ LAST }; y > 0; --y)
        if (visit(ds, x, y, side)) break;
    } else {
      for (uint8_t y{ 0 }; y < MAP_HEIGHT; ++y)
        if (visit(ds, x, y, side)) break;
    }
}
} // namespace

uint16_t opeu_count_request_cells(uint8_t* ds) {
  uint16_t count{ 0 };
  for (auto const candidate : Data(ds).arrival_candidates)
    if (candidate == 1) ++count;
  return count;
}

uint16_t opeu_request_cells(uint8_t* ds, uint8_t side, Predicate predicate) {
  auto& data{ Data(ds) };
  data.arrival_candidates.fill(0);
  auto const enabled{ [&](unsigned border) { return data.request_directions[side * 8 + border] == 1; } };
  if (enabled(0)) {
    RowRays(ds, side, predicate, HALF, MAP_HEIGHT, false);
    ColumnRays(ds, side, predicate, 0, HALF, true);
  }
  if (enabled(1)) ColumnRays(ds, side, predicate, 0, MAP_WIDTH, true);
  if (enabled(2)) {
    ColumnRays(ds, side, predicate, HALF, MAP_WIDTH, true);
    RowRays(ds, side, predicate, HALF, MAP_HEIGHT, true);
  }
  if (enabled(3)) RowRays(ds, side, predicate, 0, MAP_HEIGHT, false);
  if (enabled(4)) RowRays(ds, side, predicate, 0, MAP_HEIGHT, true);
  if (enabled(5)) {
    RowRays(ds, side, predicate, 0, HALF, false);
    ColumnRays(ds, side, predicate, 0, HALF, false);
  }
  if (enabled(6)) ColumnRays(ds, side, predicate, 0, MAP_WIDTH, false);
  if (enabled(7)) {
    ColumnRays(ds, side, predicate, HALF, MAP_WIDTH, false);
    RowRays(ds, side, predicate, 0, HALF, true);
  }
  return opeu_count_request_cells(ds);
}

uint16_t svc_reqcells(uint8_t* a0, uint8_t a1, uint8_t a2, uint8_t a3) {
  (void)a0;
  (void)a2;
  (void)a3;
  port_unhandled("reqcells (no service table)", a1, 0);
  return 0;
}
