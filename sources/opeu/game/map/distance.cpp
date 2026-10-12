// The Chebyshev distance between two map cells.
#include "map/distance.hpp"
#include "core/limits.hpp"
#include "types.hpp"

uint8_t opeu_grid_distance(uint8_t x, uint8_t y, uint8_t x2, uint8_t y2) {
  return static_cast<uint8_t>(opeu_max16(opeu_abs16_bits(static_cast<uint16_t>(x - x2)), opeu_abs16_bits(static_cast<uint16_t>(y - y2))));
}
