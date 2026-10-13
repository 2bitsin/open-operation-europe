#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_ranged_sound(uint8_t* ds, BattleEvent event, uint16_t shooter);
void opeu_battle_ranged_tracers(uint8_t* ds, BattleEvent event, uint16_t shooter, uint16_t target);
void opeu_battle_ranged_fire(uint8_t* ds, BattleEvent event, uint16_t shooter, uint16_t target);

// The services of this module.
uint16_t svc_btlrfire(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count);
