// The Chebyshev distance between two map cells.
#include "map/distance.hpp"
#include "core/limits.hpp"
#include "types.hpp"

namespace opeu::game::detail::distance {

auto GridDistance(GridCell from, GridCell to) -> std::uint8_t {
  return static_cast<std::uint8_t>(opeu_max16(opeu_abs16_bits(static_cast<std::uint16_t>(from.x - to.x)), opeu_abs16_bits(static_cast<std::uint16_t>(from.y - to.y))));
}
} // namespace opeu::game::detail::distance
