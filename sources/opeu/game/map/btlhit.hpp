#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_attack_damage(Game& game, uint16_t attacker, uint16_t target);
void     opeu_battle_apply_damage(Game& game, uint16_t attacker, uint16_t target);
void     opeu_battle_apply_bombard(Game& game, uint16_t target);
void     opeu_battle_resolve_attack(Game& game, uint16_t attacker, uint16_t target);
