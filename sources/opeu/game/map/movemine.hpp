#pragma once

#include "common/events.hpp"
#include "types.hpp"

void    opeu_move_mines(Game& game, uint8_t x, uint8_t y);
void    opeu_move_clamp(Game& game);
uint8_t opeu_move_rating(Game& game);
