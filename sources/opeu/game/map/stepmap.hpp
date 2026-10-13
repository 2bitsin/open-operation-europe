#pragma once

#include "map/grid-cell.hpp"
#include "types.hpp"

#include <cstdint>
#include <optional>

namespace opeu::game::detail::stepmap {

// The neighbour of a map cell in one of the eight directions (8 and above: the cell itself), when it lies on the map.
[[nodiscard]] auto StepOnMap(GridCell cell, std::uint8_t direction) -> std::optional<GridCell>;
} // namespace opeu::game::detail::stepmap

namespace opeu::game {
using detail::stepmap::StepOnMap;
} // namespace opeu::game

// StepOnMap through the coordinate bytes, for the callers outside game/map: moves the cell and returns 1 when the step
// stays on the map, otherwise leaves it and returns 0.
uint16_t opeu_grid_step(uint8_t* x, uint8_t* y, uint8_t direction);
