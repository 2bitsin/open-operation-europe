#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_battle_player_orders(Game& game, BattleEvent event);
uint16_t opeu_battle_side_human(Game& game, BattleEvent event, uint8_t index);
void     opeu_battle_redraw_side(Game& game, BattleEvent event, uint8_t index);
void     opeu_battle_player_turn(Game& game, BattleEvent event);

// The services of this module.
uint16_t svc_btlplay(Game& game, uint32_t service, const uint16_t* args, unsigned int count);
