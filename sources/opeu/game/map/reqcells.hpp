#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_count_request_cells(Game& game);
// The ray scan's visit of a cell (x, y) for a side: marks the cell as a candidate or not, nonzero to stop the ray.
using CellPredicate = auto(Game& game, uint8_t x, uint8_t y, uint8_t side) -> uint16_t;

uint16_t opeu_request_cells(Game& game, uint8_t side, CellPredicate& predicate);
