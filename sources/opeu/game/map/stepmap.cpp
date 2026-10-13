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
