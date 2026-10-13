#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_battle_ai_profile(Game& game, BattleEvent event, uint16_t unit, uint16_t all);
void    opeu_battle_ai_move_to(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint16_t element);
void    opeu_battle_ai_attack(Game& game, BattleEvent event, uint16_t element, uint16_t unit, uint16_t target);
void    opeu_battle_ai_attack_near(Game& game, BattleEvent event, uint16_t element, uint16_t unit);
void    opeu_battle_ai_attack_mid(Game& game, BattleEvent event, uint16_t element, uint16_t unit);
void    opeu_battle_ai_attack_far(Game& game, BattleEvent event, uint16_t element, uint16_t unit);

// The services of this module.
uint16_t svc_btlaimov(Game& game, uint32_t service, const uint16_t* args, unsigned int count);
