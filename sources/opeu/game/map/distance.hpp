#pragma once

#include "map/grid-cell.hpp"
#include "types.hpp"

#include <cstdint>

namespace opeu::game::detail::distance {

// The Chebyshev distance between two map cells.
[[nodiscard]] auto GridDistance(GridCell from, GridCell to) -> std::uint8_t;
} // namespace opeu::game::detail::distance

namespace opeu::game {
using detail::distance::GridDistance;
} // namespace opeu::game

// GridDistance for the callers outside game/map.
uint8_t opeu_grid_distance(uint8_t x, uint8_t y, uint8_t x2, uint8_t y2);
