#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_battle_type_skill(uint8_t* ds, BattleEvent event, uint16_t element);
uint16_t opeu_battle_ranged_hits(uint8_t* ds, BattleEvent event, uint16_t shooter, uint16_t target);
uint16_t opeu_battle_ranged_power(uint8_t* ds, BattleEvent event, uint16_t shooter, uint16_t target);
uint16_t opeu_battle_ranged_defense(uint8_t* ds, BattleEvent event, uint16_t element);
uint16_t opeu_battle_ranged_damage(uint8_t* ds, BattleEvent event, uint16_t shooter, uint16_t target, uint16_t bunker);

// The services of this module.
uint16_t svc_btlrcalc(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count);
