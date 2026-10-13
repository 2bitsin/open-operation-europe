// A step of a map cell that is taken only when it stays on the map.
#include "map/stepmap.hpp"
#include "common/map-grid.hpp"
#include "map/step.hpp"
#include "types.hpp"

namespace opeu::game::detail::stepmap {

auto StepOnMap(GridCell cell, std::uint8_t direction) -> std::optional<GridCell> {
  auto const next{ opeu_grid_step_unchecked(cell, direction) };
  if (!OnMap(next.x, next.y)) return std::nullopt;
  return next;
}
} // namespace opeu::game::detail::stepmap

using namespace opeu::game;

uint16_t opeu_grid_step(uint8_t* x, uint8_t* y, uint8_t direction) {
  auto const next{ StepOnMap({ *x, *y }, direction) };
  if (!next) return 0;
  *x = next->x;
  *y = next->y;
  return 1;
}
