#pragma once

#include "common/events.hpp"
#include "common/grid-cell.hpp"
#include "map/path-grid.hpp"
#include "types.hpp"

#include <cstdint>

namespace opeu::game::detail::movecost {

// A unit's way between two map cells.
struct MoveRoute {
  GridCell      from;
  GridCell      to;
  std::uint16_t unit;
};
} // namespace opeu::game::detail::movecost

namespace opeu::game {
using detail::movecost::MoveRoute;
} // namespace opeu::game

uint8_t opeu_move_cost(Game& game, opeu::game::GridCell cell, uint16_t unit);
// Whether the unit can reach the route's end from its start, which must lie on the map; fills the path grid's costs.
[[nodiscard]] auto opeu_move_reachable(Game& game, opeu::game::PathGrid const& path, opeu::game::MoveRoute const& route) -> bool;
