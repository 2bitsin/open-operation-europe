// Deputies: a new deputy's identity and starting ratings.
#pragma once

#include "types.hpp"

uint8_t opeu_choose_deputy_identity(uint8_t* ds, uint8_t side);
void    opeu_initialize_deputy(uint8_t* ds, uint16_t person);
