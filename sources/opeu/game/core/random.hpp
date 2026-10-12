// The game's random-number generator.
#pragma once

#include "types.hpp"

extern uint32_t opeu_random_state;
uint16_t        opeu_rand_next(void);
void            opeu_rand_seed(uint16_t seed);
uint16_t        opeu_rand_bounded(uint16_t bound);
