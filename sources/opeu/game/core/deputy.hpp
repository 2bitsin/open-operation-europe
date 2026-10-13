// Deputies: a new deputy's identity and starting ratings.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_choose_deputy_identity(Game& game, uint8_t side);
void    opeu_initialize_deputy(Game& game, uint16_t person);
