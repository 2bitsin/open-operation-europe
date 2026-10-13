#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_battle_player_orders(Game& game);
uint16_t opeu_battle_side_human(Game& game, uint8_t index);
void     opeu_battle_redraw_side(Game& game, uint8_t index);
void     opeu_battle_player_turn(Game& game);
