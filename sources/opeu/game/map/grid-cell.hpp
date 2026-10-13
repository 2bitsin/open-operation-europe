// A cell of the 40 x 40 strategic map or of the 14 x 14 battle field: two byte coordinates, so a step off the top or
// left edge wraps to a large value.
#pragma once

#include <cstdint>

namespace opeu::game::detail::grid_cell {

struct GridCell {
  std::uint8_t x;
  std::uint8_t y;

  friend constexpr auto operator==(GridCell, GridCell) -> bool = default;
};

// A move from one cell to another.
struct CellMove {
  GridCell from;
  GridCell to;
};

// A step of a path search: the cell stepped to and its accumulated cost.
struct PathStep {
  GridCell      cell;
  std::uint16_t cost;
};
} // namespace opeu::game::detail::grid_cell

namespace opeu::game {
using detail::grid_cell::CellMove;
using detail::grid_cell::GridCell;
using detail::grid_cell::PathStep;
} // namespace opeu::game
