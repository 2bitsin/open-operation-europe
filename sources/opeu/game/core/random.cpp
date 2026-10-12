// The game's random-number generator: a 32-bit linear congruential state.
#include "core/random.hpp"
#include "types.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

uint32_t opeu_random_state;

uint16_t opeu_rand_next(void) {
  opeu_random_state = opeu_random_state * 1103515245UL + 12345UL;
  return (uint16_t)((opeu_random_state >> 16) & 0x7fff);
}

// The seed is a word; the state's high word becomes zero.
void opeu_rand_seed(uint16_t seed) {
  opeu_random_state = seed;
}

// A number below bound; a zero bound gives 0 without advancing the state.
uint16_t opeu_rand_bounded(uint16_t bound) {
  if (bound == 0) return bound;
  return opeu_rand_next() % bound;
}

auto port_routine_opeu_rand_bounded(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return port_rand_bounded(ds, Arg(args, count, 0));
}
