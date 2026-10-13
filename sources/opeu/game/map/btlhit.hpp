#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_attack_damage(Game& game, BattleEvent event, uint16_t attacker, uint16_t target);
void     opeu_battle_apply_damage(Game& game, BattleEvent event, uint16_t attacker, uint16_t target);
void     opeu_battle_apply_bombard(Game& game, BattleEvent event, uint16_t target);
void     opeu_battle_resolve_attack(Game& game, BattleEvent event, uint16_t attacker, uint16_t target);

// The services of this module.
uint16_t svc_btlhit(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
