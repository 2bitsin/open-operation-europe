#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_fire_hits(Game& game, uint16_t attacker, uint16_t target);
uint16_t opeu_battle_fire_power(Game& game, uint16_t attacker, uint16_t target);
uint16_t opeu_battle_fire_defense(Game& game, uint16_t element);
