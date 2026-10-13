// One step of a map position toward a target along the cheapest neighbour of the 40 x 40 path-cost grid. Of the
// eight directions that stay on the map the lowest cost wins; on a tie, a neighbour that is not farther from the target
// on either axis than the start replaces the earlier one.
#include "map/toward.hpp"
#include "common/map-grid.hpp"
#include "map/stepmap.hpp"
#include "types.hpp"

#include <cstdlib>
#include <ranges>

using namespace opeu::game;

namespace {
// Whether the cell is no farther from the goal on either axis than the start.
[[nodiscard]] auto NoFarther(GridCell cell, GridCell from, GridCell goal) -> bool {
  return std::abs(cell.x - goal.x) <= std::abs(from.x - goal.x) && std::abs(cell.y - goal.y) <= std::abs(from.y - goal.y);
}
} // namespace

auto opeu_grid_step_toward(PathGrid const& path, GridCell from, GridCell goal) -> PathStep {
  PathStep best{ from, PathGrid::UNREACHED };
  for (auto const next : std::views::iota(uint8_t{ 0 }, GRID_DIRECTIONS) | std::views::transform([&](uint8_t direction) { return StepOnMap(from, direction); })) {
    if (!next) continue;
    auto const cost{ path.Cost(*next) };
    if (cost < best.cost || (cost == best.cost && NoFarther(*next, from, goal))) best = { *next, cost };
  }
  return best;
}
