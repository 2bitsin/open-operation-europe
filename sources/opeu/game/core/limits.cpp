// Limits and signed-word helpers.
#include "core/limits.hpp"
#include "types.hpp"

#include "glue.hpp"
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

uint16_t opeu_negative_to_minus_one(uint16_t value) {
  return (int16_t)value < 0 ? 0xffff : value;
}

// The routines other modules reach through the link table.
auto port_routine_opeu_max16(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)ds;
  return opeu_max16(Arg(args, count, 0), Arg(args, count, 1));
}

auto port_routine_opeu_min16(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)ds;
  return opeu_min16(Arg(args, count, 0), Arg(args, count, 1));
}
