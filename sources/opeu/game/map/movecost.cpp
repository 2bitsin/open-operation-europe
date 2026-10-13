// A unit's movement cost into a cell, and whether a unit can reach a cell.
//
// The cost is 255 (impassable) for a cell the unit's side marked or holds, otherwise the highest terrain cost of the
// unit's elements (move_costs by terrain and movement class) times the map scale, truncated to a byte.
//
// The reachability test floods out from the start cell in order of accumulated cost through the path grid's queue,
// kept sorted by cost (insertion from the tail), and its cost grid.
#include "map/movecost.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/game.hpp"
#include "common/map-grid.hpp"
#include "core/recindex.hpp"
#include "map/cellflag.hpp"
#include "map/grid.hpp"
#include "map/occupy.hpp"
#include "map/stepmap.hpp"
#include "types.hpp"

#include <algorithm>
#include <cstddef>
#include <ranges>

using namespace opeu::game;

namespace {
constexpr unsigned TERRAIN_COLUMNS{ 5 }; // move_costs: one row per terrain, one column per movement class

// Inserts the cell into the queue's first `count` entries after the last one no dearer than it.
void InsertByCost(PathGrid const& path, std::size_t count, uint16_t cell) {
  auto const costs{ path.Costs() };
  auto const queue{ path.Queue().first(count + 1) };
  auto const queued{ queue.first(count) };
  auto const no_dearer{ std::ranges::find_if(queued | std::views::reverse, [&](uint16_t entry) { return costs[entry] <= costs[cell]; }).base() };
  std::shift_right(no_dearer, queue.end(), 1);
  *no_dearer = cell;
}
} // namespace

uint8_t opeu_move_cost(Game& game, GridCell cell, uint16_t unit) {
  auto const& data{ Data(game) };
  auto const  side{ opeu_unit_side(unit) };
  if (opeu_cell_marked(game, cell, side) || opeu_cell_side(game, cell.x, cell.y) == side) return IMPASSABLE_COST;
  auto const terrain{ opeu_map_code(game, cell.x, cell.y) };
  uint8_t    cost{ 0 };
  for (auto const& element : Elements(data, Record<Unit>(data, unit))) {
    auto const move_class{ data.move_class[data.element_types[element.type].category] };
    cost = std::max(cost, data.move_costs[terrain * TERRAIN_COLUMNS + move_class]);
  }
  if (cost != IMPASSABLE_COST) cost = static_cast<uint8_t>(cost * data.map_scale);
  return cost;
}

auto opeu_move_reachable(Game& game, PathGrid const& path, MoveRoute const& route) -> bool {
  auto const  queue{ path.Queue() };
  auto const  costs{ path.Costs() };
  std::size_t head{ 0 };
  std::size_t tail{ 1 };
  queue[0] = PathGrid::Index(route.from);
  std::ranges::fill(costs, PathGrid::UNREACHED);
  costs[queue[0]] = 0;
  // The queue grows while it is walked: a hand loop.
  while (tail != head) {
    GridCell const from{ static_cast<uint8_t>(queue[head] % MAP_WIDTH), static_cast<uint8_t>(queue[head] / MAP_WIDTH) };
    auto const     cost{ path.Cost(from) };
    for (auto const direction : std::views::iota(uint8_t{ 0 }, GRID_DIRECTIONS)) {
      auto const next{ StepOnMap(from, direction) };
      if (!next || !opeu_map_code(game, next->x, next->y)) continue;
      if (*next == route.to) return true;
      auto const cell{ PathGrid::Index(*next) };
      if (costs[cell] != PathGrid::UNREACHED) continue;
      auto const step{ opeu_move_cost(game, *next, route.unit) };
      if (step == IMPASSABLE_COST) continue;
      costs[cell] = static_cast<uint16_t>(step + cost);
      InsertByCost(path, tail, cell);
      ++tail;
    }
    ++head;
  }
  return false;
}
