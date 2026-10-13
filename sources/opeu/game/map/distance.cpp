// The Chebyshev distance between two map cells.
#include "map/distance.hpp"
#include "core/limits.hpp"
#include "types.hpp"

namespace opeu::game::detail::distance {

auto GridDistance(GridCell from, GridCell to) -> std::uint8_t {
  return static_cast<std::uint8_t>(opeu_max16(opeu_abs16_bits(static_cast<std::uint16_t>(from.x - to.x)), opeu_abs16_bits(static_cast<std::uint16_t>(from.y - to.y))));
}
} // namespace opeu::game::detail::distance

uint8_t opeu_grid_distance(uint8_t x, uint8_t y, uint8_t x2, uint8_t y2) {
  return opeu::game::GridDistance({ x, y }, { x2, y2 });
}
