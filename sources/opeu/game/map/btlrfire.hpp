#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_ranged_sound(Game& game, uint16_t shooter);
void opeu_battle_ranged_tracers(Game& game, uint16_t shooter, uint16_t target);
void opeu_battle_ranged_fire(Game& game, uint16_t shooter, uint16_t target);
