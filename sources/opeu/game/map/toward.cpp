// One step of a map position toward a target along the cheapest neighbour of the 40 x 40 path-cost grid in the far
// data segment. Of the eight directions that stay on the map the lowest cost wins; on a tie, a neighbour that is not
// farther from the target on either axis than the start replaces the earlier one. The chosen cell is stored back and
// its cost returned.
#include "map/toward.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "map/stepmap.hpp"
#include "types.hpp"

#include <cstdlib>

using namespace opeu::game;

namespace {
constexpr uint16_t PATH_COSTS{ 0x0c80 }; // the cost words in the far data
constexpr uint8_t  DIRECTIONS{ 8 };

[[nodiscard]] auto PathCost(const uint8_t* far_data, uint8_t x, uint8_t y) -> uint16_t {
  auto const at{ PATH_COSTS + (y * MAP_WIDTH + x) * 2 };
  return static_cast<uint16_t>(far_data[at] | far_data[at + 1] << 8);
}
} // namespace

// x and y are the DS offsets of the position's coordinate bytes.
uint16_t opeu_grid_step_toward(uint8_t* ds, const uint8_t* far_data, uint16_t x, uint16_t y, uint8_t tx, uint8_t ty) {
  auto const distance_x{ std::abs(Byte(ds, x) - tx) };
  auto const distance_y{ std::abs(Byte(ds, y) - ty) };
  uint16_t   best_cost{ 0xffff };
  uint8_t    best_x{ Byte(ds, x) };
  uint8_t    best_y{ Byte(ds, y) };
  for (uint8_t direction{ 0 }; direction < DIRECTIONS; ++direction) {
    uint8_t next_x{ Byte(ds, x) };
    uint8_t next_y{ Byte(ds, y) };
    if (!opeu_grid_step(&next_x, &next_y, direction)) continue;
    auto const cost{ PathCost(far_data, next_x, next_y) };
    if (cost < best_cost
        || (cost == best_cost && std::abs(next_x - tx) <= distance_x && std::abs(next_y - ty) <= distance_y)) {
      best_x    = next_x;
      best_y    = next_y;
      best_cost = cost;
    }
  }
  Byte(ds, x) = best_x;
  Byte(ds, y) = best_y;
  return best_cost;
}
