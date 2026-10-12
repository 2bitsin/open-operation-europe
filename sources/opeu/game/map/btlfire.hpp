#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_fire_hits(uint8_t* ds, BattleEvent event, uint16_t attacker, uint16_t target);
uint16_t opeu_battle_fire_power(uint8_t* ds, BattleEvent event, uint16_t attacker, uint16_t target);
uint16_t opeu_battle_fire_defense(uint8_t* ds, BattleEvent event, uint16_t element);

// The services of this module.
uint16_t svc_btlfire(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count);
