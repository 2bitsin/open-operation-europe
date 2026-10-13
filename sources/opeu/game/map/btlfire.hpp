#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_fire_hits(Game& game, BattleEvent event, uint16_t attacker, uint16_t target);
uint16_t opeu_battle_fire_power(Game& game, BattleEvent event, uint16_t attacker, uint16_t target);
uint16_t opeu_battle_fire_defense(Game& game, BattleEvent event, uint16_t element);

// The services of this module.
uint16_t svc_btlfire(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
