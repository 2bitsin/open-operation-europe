#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_ranged_sound(Game& game, BattleEvent event, uint16_t shooter);
void opeu_battle_ranged_tracers(Game& game, BattleEvent event, uint16_t shooter, uint16_t target);
void opeu_battle_ranged_fire(Game& game, BattleEvent event, uint16_t shooter, uint16_t target);

// The services of this module.
uint16_t svc_btlrfire(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
