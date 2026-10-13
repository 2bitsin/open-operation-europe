// Limits and signed-word helpers.
#include "core/limits.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

uint16_t opeu_min16(uint16_t a, uint16_t b) {
  return a < b ? a : b;
}

uint16_t opeu_max16(uint16_t a, uint16_t b) {
  return a > b ? a : b;
}

// The absolute value of a signed word; abs(-32768) stays 0x8000.
uint16_t opeu_abs16_bits(uint16_t value) {
  return (uint16_t)((int16_t)value < 0 ? -(int16_t)value : (int16_t)value);
}
