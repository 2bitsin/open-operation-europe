#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_battle_player_orders(uint8_t* ds, BattleEvent event);
uint16_t opeu_battle_side_human(uint8_t* ds, BattleEvent event, uint8_t index);
void     opeu_battle_redraw_side(uint8_t* ds, BattleEvent event, uint8_t index);
void     opeu_battle_player_turn(uint8_t* ds, BattleEvent event);

// The services of this module.
uint16_t svc_btlplay(uint8_t* ds, uint32_t service, const uint16_t* args, unsigned int count);
