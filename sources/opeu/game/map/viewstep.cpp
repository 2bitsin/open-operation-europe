// A step of a map cell that is taken only when it stays on the map and inside the 12 x 12 cell viewport.
#include "types.hpp"
#include "map/viewstep.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "common/map-grid.hpp"
#include "map/step.hpp"

using namespace opeu::game;

namespace {
constexpr int16_t VIEW_CELLS{ 12 };
}

// x and y are the DS offsets of the cell's coordinate bytes; returns 1 when the step was taken.
uint16_t opeu_grid_step_viewport(uint8_t* ds, uint16_t x, uint16_t y,
                                 uint8_t direction) {
  auto const& data{ Data(ds) };
  uint8_t     next_x{ Byte(ds, x) };
  uint8_t     next_y{ Byte(ds, y) };
  opeu_grid_step_unchecked(&next_x, &next_y, direction);
  auto const view_x{ static_cast<int16_t>(data.viewport_x) };
  auto const view_y{ static_cast<int16_t>(data.viewport_y) };
  if (!OnMap(next_x, next_y) || next_x < view_x || next_y < view_y ||
      next_x >= static_cast<int16_t>(view_x + VIEW_CELLS) ||
      next_y >= static_cast<int16_t>(view_y + VIEW_CELLS))
    return 0;
  Byte(ds, x) = next_x;
  Byte(ds, y) = next_y;
  return 1;
}
