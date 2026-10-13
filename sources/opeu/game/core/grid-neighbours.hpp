// The cells around a map cell that lie on the map.
#pragma once

#include "common/map-grid.hpp"
#include "map/grid-cell.hpp"
#include "map/stepmap.hpp"

#include <cstdint>
#include <optional>
#include <ranges>

namespace opeu::game::detail::grid_neighbours {

// The on-map cells the first `directions` directions lead to from the cell, in direction order: GRID_DIRECTIONS gives
// the eight around it, GRID_DIRECTIONS_AND_CENTER those and the cell itself.
[[nodiscard]] inline auto CellsAround(GridCell cell, std::uint8_t directions = GRID_DIRECTIONS) {
  return std::views::iota(std::uint8_t{ 0 }, directions) | std::views::transform([cell](std::uint8_t direction) { return StepOnMap(cell, direction); })
         | std::views::filter([](std::optional<GridCell> const& step) { return step.has_value(); }) | std::views::transform([](std::optional<GridCell> const& step) { return *step; });
}
} // namespace opeu::game::detail::grid_neighbours

namespace opeu::game {
using detail::grid_neighbours::CellsAround;
} // namespace opeu::game
