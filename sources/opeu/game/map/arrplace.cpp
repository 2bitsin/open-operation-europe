// Picks an arrival cell for a side from the candidate grid that the request-cell scan fills. Each candidate after the
// first replaces the pick with probability 1/n, n being the number of candidates.
#include "map/arrplace.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "map/cellpred.hpp"
#include "map/reqcells.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"

using namespace opeu::game;

namespace {
constexpr uint16_t COUNT_CELLS{ 0 }; // the request-cell scan: side, predicate offset, predicate segment
// The far address by which the request-cell service knows each scan predicate.
constexpr uint16_t PREDICATE_SEGMENT{ 0x145c };
constexpr uint16_t ARRIVAL_PREDICATE{ 0x1152 };
constexpr uint16_t REQUEST_PREDICATE{ 0x1114 };
constexpr uint8_t  NONE{ 255 };
} // namespace

// x and y: DS offsets of the bytes that receive the cell; returns 0 when there is no candidate.
uint16_t opeu_arrival_locate(Game& game, uint16_t (*event)(Game&, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t), uint16_t x, uint16_t y, uint8_t side) {
  auto const count{ event(game, COUNT_CELLS, side, ARRIVAL_PREDICATE, PREDICATE_SEGMENT, 0) };
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

uint16_t svc_arrplace(Game& game, uint16_t id, uint16_t side, uint16_t off, uint16_t seg, uint16_t unused) {
  (void)unused;
  if (id != COUNT_CELLS) {
    port_unhandled("arrplace", id, 0);
    return 0;
  }
  if (seg == PREDICATE_SEGMENT && off == ARRIVAL_PREDICATE) return opeu_request_cells(game, static_cast<uint8_t>(side), opeu_arrival_cell_allowed);
  if (seg == PREDICATE_SEGMENT && off == REQUEST_PREDICATE) return opeu_request_cells(game, static_cast<uint8_t>(side), opeu_request_cell_allowed);
  port_fatal("arrplace: %04x:%04x is not a request-cell predicate", seg, off);
  return 0;
}
