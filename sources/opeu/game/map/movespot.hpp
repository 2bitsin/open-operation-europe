#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_move_spot(Game& game);
uint16_t opeu_move_enemy_adjacent(Game& game);
void     opeu_move_mines_finish(Game& game);
