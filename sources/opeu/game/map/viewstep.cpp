// A step of a map cell that is taken only when it stays on the map and inside the 12 x 12 cell viewport.
#include "map/viewstep.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "map/step.hpp"
#include "types.hpp"

namespace opeu::game::detail::viewstep {
namespace {
constexpr std::int16_t VIEW_CELLS{ 12 };

[[nodiscard]] auto InViewport(DataSegment const& data, GridCell cell) -> bool {
  auto const view_x{ static_cast<std::int16_t>(data.viewport_x) };
  auto const view_y{ static_cast<std::int16_t>(data.viewport_y) };
  return cell.x >= view_x && cell.y >= view_y && cell.x < static_cast<std::int16_t>(view_x + VIEW_CELLS) && cell.y < static_cast<std::int16_t>(view_y + VIEW_CELLS);
}
} // namespace

auto StepInViewport(DataSegment const& data, GridCell cell, std::uint8_t direction) -> std::optional<GridCell> {
  auto const next{ opeu_grid_step_unchecked(cell, direction) };
  if (!OnMap(next.x, next.y) || !InViewport(data, next)) return std::nullopt;
  return next;
}
} // namespace opeu::game::detail::viewstep
