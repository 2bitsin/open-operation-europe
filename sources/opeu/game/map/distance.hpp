#pragma once

#include "common/grid-cell.hpp"
#include "types.hpp"

#include <cstdint>

namespace opeu::game::detail::distance {

// The Chebyshev distance between two map cells.
[[nodiscard]] auto GridDistance(GridCell from, GridCell to) -> std::uint8_t;
} // namespace opeu::game::detail::distance

namespace opeu::game {
using detail::distance::GridDistance;
} // namespace opeu::game
