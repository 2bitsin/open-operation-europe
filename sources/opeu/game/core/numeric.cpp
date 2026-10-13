// Arithmetic helpers: scaled products and capped additions and subtractions on bytes and words.
#include "core/numeric.hpp"
#include "core/limits.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

// a * b / denominator: the full 32-bit product divided as a signed number; 0xffff for a zero denominator.
uint16_t opeu_mul_div(uint16_t a, uint16_t b, uint16_t denominator) {
  if (!denominator) return 0xffff;
  return (uint16_t)((int32_t)((uint32_t)a * b) / (int32_t)denominator);
}

// a percent of b.
uint16_t opeu_percent(uint16_t a, uint16_t b) {
  return opeu_mul_div(a, b, 100);
}

// Adds amount to the byte, capped at limit; the sum wraps at 16 bits before the cap and the result is truncated to
// the byte.
uint8_t opeu_add_byte(uint16_t amount, uint16_t limit, uint8_t* value) {
  *value = (uint8_t)opeu_min16(amount + *value, limit);
  return *value;
}

// Takes amount from the byte, stopping at 0.
uint8_t opeu_sub_byte(uint16_t amount, uint8_t* value) {
  *value -= (uint8_t)opeu_min16(*value, amount);
  return *value;
}

// Adds amount to the word, capped at limit; unlike the byte helper the sum does not wrap.
uint16_t opeu_add_word(uint16_t amount, uint16_t limit, uint16_t* value) {
  int32_t sum = (int32_t)amount + *value;
  if (sum > limit) sum = limit;
  return *value = (uint16_t)sum;
}

// Takes amount from the word, stopping at 0.
uint16_t opeu_sub_word(uint16_t amount, uint16_t* value) {
  *value -= opeu_min16(*value, amount);
  return *value;
}
