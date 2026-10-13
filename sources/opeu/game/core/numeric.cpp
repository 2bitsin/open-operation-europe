// Arithmetic helpers: scaled products and capped additions and subtractions on bytes and words.
#include "core/numeric.hpp"
#include "core/limits.hpp"
#include "types.hpp"

#include <algorithm>

#include "common/game.hpp"

// a * b / denominator: the full 32-bit product divided as a signed number; 0xffff for a zero denominator.
uint16_t opeu_mul_div(uint16_t a, uint16_t b, uint16_t denominator) {
  if (!denominator) return 0xffff;
  return (uint16_t)((int32_t)((uint32_t)a * b) / (int32_t)denominator);
}

// a percent of b.
uint16_t opeu_percent(uint16_t a, uint16_t b) {
  return opeu_mul_div(a, b, 100);
}

// value plus amount, capped at limit; the sum wraps at 16 bits before the cap and the result is truncated to the byte.
auto AddByte(uint8_t value, uint16_t amount, uint16_t limit) -> uint8_t {
  return static_cast<uint8_t>(opeu_min16(static_cast<uint16_t>(amount + value), limit));
}

// value less amount, stopping at 0.
auto SubByte(uint8_t value, uint16_t amount) -> uint8_t {
  return static_cast<uint8_t>(value - opeu_min16(value, amount));
}

// value plus amount, capped at limit; unlike the byte helper the sum does not wrap.
auto AddWord(uint16_t value, uint16_t amount, uint16_t limit) -> uint16_t {
  return static_cast<uint16_t>(std::min<int32_t>(int32_t{ amount } + value, limit));
}

// value less amount, stopping at 0.
auto SubWord(uint16_t value, uint16_t amount) -> uint16_t {
  return static_cast<uint16_t>(value - opeu_min16(value, amount));
}
