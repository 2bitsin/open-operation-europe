#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_battle_execute_order(Game& game, BattleEvent event, uint16_t element, uint8_t order);
void    opeu_battle_attack_or_hold(Game& game, BattleEvent event, uint16_t element, uint16_t target);
void    opeu_battle_continue_order(Game& game, BattleEvent event, uint16_t element);
void    opeu_battle_advance(Game& game, BattleEvent event, uint16_t element);
void    opeu_battle_seek_target(Game& game, BattleEvent event, uint16_t element);

// The services of this module.
uint16_t svc_btlorder(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
