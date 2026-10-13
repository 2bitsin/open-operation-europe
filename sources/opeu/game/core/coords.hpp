// The map inspection mode: a cursor over the map, its panels and the regiment orders it leads to.
#pragma once

#include "common/events.hpp"
#include "common/grid-cell.hpp"

namespace opeu::game::detail::coords {

// Where an inspection ended: the cell under the cursor, and whether the player picked a cell (or ordered a regiment)
// rather than left.
struct Inspection {
  GridCell cell;
  bool     picked;
};

// Runs the inspection from the cell `start`.
[[nodiscard]] auto InspectMap(Game& game, GridCell start) -> Inspection;
} // namespace opeu::game::detail::coords

namespace opeu::game {
using detail::coords::Inspection;
using detail::coords::InspectMap;
} // namespace opeu::game
