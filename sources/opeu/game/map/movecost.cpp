// A unit's movement cost into a cell, clearing a cell of the far path-cost grid, and whether a unit can reach a cell.
//
// The cost is 255 (impassable) for a cell the unit's side marked or holds, otherwise the highest terrain cost of the
// unit's elements (move_costs by terrain and movement class) times the map scale, truncated to a byte.
//
// The reachability test floods out from the start cell in order of accumulated cost: a queue of cell indices kept
// sorted by cost (insertion from the tail) and a 40 x 40 grid of costs, 0xffff for cells not reached yet, both in the
// far data segment (the queue from word 0, the grid from word 0x640).
#include "map/movecost.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/map-grid.hpp"
#include "core/recindex.hpp"
#include "map/cellflag.hpp"
#include "map/grid.hpp"
#include "map/occupy.hpp"
#include "map/stepmap.hpp"
#include "types.hpp"

#include <algorithm>

using namespace opeu::game;

namespace {
constexpr uint16_t UNREACHED{ 0xffff };
constexpr uint16_t PATH_COSTS{ 0x0c80 }; // the grid's byte offset in the far data
constexpr unsigned GRID_WORDS{ 0x640 };  // the grid's word offset: the queue comes first
constexpr unsigned CELL_COUNT{ MAP_WIDTH * MAP_HEIGHT };
constexpr unsigned TERRAIN_COLUMNS{ 5 }; // move_costs: one row per terrain, one column per movement class
} // namespace

uint8_t opeu_move_cost(uint8_t* ds, uint8_t x, uint8_t y, uint16_t unit) {
  auto const& data{ Data(ds) };
  auto const  side{ opeu_unit_side(unit) };
  if (opeu_cell_marked(ds, x, y, side) || opeu_cell_side(ds, x, y) == side) return IMPASSABLE;
  auto const terrain{ opeu_map_code(ds, x, y) };
  uint8_t    cost{ 0 };
  for (auto const& element : Elements(data, Record<Unit>(data, unit))) {
    auto const move_class{ data.move_class[data.element_types[element.type].category] };
    cost = std::max(cost, data.move_costs[terrain * TERRAIN_COLUMNS + move_class]);
  }
  if (cost != IMPASSABLE) cost = static_cast<uint8_t>(cost * data.map_scale);
  return cost;
}

void opeu_move_clear(uint8_t* far_data, uint8_t x, uint8_t y) {
  auto const at{ static_cast<uint16_t>(PATH_COSTS + (y * MAP_WIDTH + x) * 2) };
  far_data[at]                            = 0xff;
  far_data[static_cast<uint16_t>(at + 1)] = 0xff;
}

// far_words: the far data segment as words. The start cell must lie on the map.
uint16_t opeu_move_reachable(uint8_t* ds, uint16_t* far_words, uint8_t x, uint8_t y, uint8_t tx, uint8_t ty, uint16_t unit) {
  auto* const queue{ far_words };
  auto* const costs{ far_words + GRID_WORDS };
  unsigned    head{ 0 };
  unsigned    tail{ 1 };
  queue[0] = static_cast<uint16_t>(x + y * MAP_WIDTH);
  std::fill(costs, costs + CELL_COUNT, UNREACHED);
  costs[queue[0]] = 0;
  while (tail != head) {
    auto const from_x{ static_cast<uint8_t>(queue[head] % MAP_WIDTH) };
    auto const from_y{ static_cast<uint8_t>(queue[head] / MAP_WIDTH) };
    auto const cost{ costs[from_y * MAP_WIDTH + from_x] };
    for (uint8_t direction{ 0 }; direction < GRID_DIRECTIONS; direction++) {
      uint8_t next_x{ from_x };
      uint8_t next_y{ from_y };
      if (!opeu_grid_step(&next_x, &next_y, direction) || !opeu_map_code(ds, next_x, next_y)) continue;
      if (next_x == tx && next_y == ty) return 1;
      auto const cell{ static_cast<uint16_t>(next_y * MAP_WIDTH + next_x) };
      if (costs[cell] != UNREACHED) continue;
      auto const step{ opeu_move_cost(ds, next_x, next_y, unit) };
      if (step == IMPASSABLE) continue;
      costs[cell] = static_cast<uint16_t>(step + cost);
      // Insert by cost: shift dearer entries up one place from the tail.
      auto at{ tail };
      for (;;) {
        queue[at] = queue[at - 1];
        if (costs[queue[at - 1]] <= costs[cell]) {
          queue[at] = cell;
          tail++;
          break;
        }
        at--;
      }
    }
    head++;
  }
  return 0;
}
