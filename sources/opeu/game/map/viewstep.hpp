#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "map/grid-cell.hpp"
#include "types.hpp"

#include <cstdint>
#include <optional>

namespace opeu::game::detail::viewstep {

// The neighbour of a map cell in one of the eight directions, when it lies on the map and inside the viewport.
[[nodiscard]] auto StepInViewport(DataSegment const& data, GridCell cell, std::uint8_t direction) -> std::optional<GridCell>;
} // namespace opeu::game::detail::viewstep

namespace opeu::game {
using detail::viewstep::StepInViewport;
} // namespace opeu::game

// StepInViewport through the cell's coordinate bytes at the DS offsets x and y, for the callers outside game/map:
// returns 1 when the step was taken.
uint16_t opeu_grid_step_viewport(Game& game, uint16_t x, uint16_t y, uint8_t direction);
