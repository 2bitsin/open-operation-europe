#pragma once

#include "common/events.hpp"
#include "types.hpp"

void    opeu_map_mark_10(Game& game, uint8_t x, uint8_t y);
void    opeu_map_mark_5(Game& game, uint8_t x, uint8_t y);
uint8_t opeu_map_code(Game const& game, uint8_t x, uint8_t y);
