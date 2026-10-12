// Picks an arrival cell for a side from the candidate grid that the request-cell scan fills. Each candidate after the
// first replaces the pick with probability 1/n, n being the number of candidates.
#include "types.hpp"
#include "map/arrplace.hpp"
#include "common/data-segment.hpp"
#include "common/random.hpp"
#include "map/cellpred.hpp"
#include "map/reqcells.hpp"

#include "port.hpp"

using namespace opeu::game;

namespace {
constexpr uint16_t COUNT_CELLS{
  0
}; // the request-cell scan: side, predicate offset, predicate segment
// The far address by which the request-cell service knows each scan predicate.
constexpr uint16_t PREDICATE_SEGMENT{ 0x145c };
constexpr uint16_t ARRIVAL_PREDICATE{ 0x1152 };
constexpr uint16_t REQUEST_PREDICATE{ 0x1114 };
constexpr uint8_t  NONE{ 255 };
} // namespace

// x and y: DS offsets of the bytes that receive the cell; returns 0 when there is no candidate.
uint16_t opeu_arrival_locate(uint8_t* ds,
                             uint16_t (*event)(uint8_t*, uint16_t, uint16_t,
                                               uint16_t, uint16_t, uint16_t),
                             uint16_t x, uint16_t y, uint8_t side) {
  auto const count{ event(ds, COUNT_CELLS, side, ARRIVAL_PREDICATE,
                          PREDICATE_SEGMENT, 0) };
  if (!count)
    return 0;
  auto const& data{ Data(ds) };
  ds[x] = NONE;
  for (uint8_t cell_y{ 0 }; cell_y < MAP_HEIGHT; ++cell_y)
    for (uint8_t cell_x{ 0 }; cell_x < MAP_WIDTH; ++cell_x)
      if (data.arrival_candidates[cell_y * MAP_WIDTH + cell_x] == 1 &&
          (ds[x] == NONE || RandomBelow(ds, count) == 0)) {
        ds[x] = cell_x;
        ds[y] = cell_y;
      }
  return 1;
}

uint16_t svc_arrplace(uint8_t* ds, uint16_t id, uint16_t side, uint16_t off,
                      uint16_t seg, uint16_t unused) {
  (void)unused;
  if (id != COUNT_CELLS) {
    port_unhandled("arrplace", id, 0);
    return 0;
  }
  if (seg == PREDICATE_SEGMENT && off == ARRIVAL_PREDICATE)
    return opeu_request_cells(ds, static_cast<uint8_t>(side),
                              opeu_arrival_cell_allowed);
  if (seg == PREDICATE_SEGMENT && off == REQUEST_PREDICATE)
    return opeu_request_cells(ds, static_cast<uint8_t>(side),
                              opeu_request_cell_allowed);
  port_fatal("arrplace: %04x:%04x is not a request-cell predicate", seg, off);
  return 0;
}
