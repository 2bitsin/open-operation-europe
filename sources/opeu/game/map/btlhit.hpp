#pragma once

#include "types.hpp"
#include "common/events.hpp"

uint16_t opeu_battle_attack_damage(uint8_t* ds, BattleEvent event,
                                   uint16_t attacker, uint16_t target);
void opeu_battle_apply_damage(uint8_t* ds, BattleEvent event, uint16_t attacker,
                              uint16_t target);
void opeu_battle_apply_bombard(uint8_t* ds, BattleEvent event, uint16_t target);
void opeu_battle_resolve_attack(uint8_t* ds, BattleEvent event,
                                uint16_t attacker, uint16_t target);

// The services of this module.
uint16_t svc_btlhit(uint8_t* ds, uint32_t address, const uint16_t* args,
                    unsigned int count);
