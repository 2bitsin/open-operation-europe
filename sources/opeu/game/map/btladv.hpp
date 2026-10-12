#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_exit_to_map(uint8_t* ds, BattleEvent event, uint16_t element, uint8_t x, uint8_t y);
void opeu_battle_element_withdraw(uint8_t* ds, BattleEvent event, uint16_t element);
void opeu_battle_withdraw_step(uint8_t* ds, BattleEvent event, uint16_t element);
void opeu_battle_hold_step(uint8_t* ds, BattleEvent event, uint16_t element);
void opeu_battle_element_step(uint8_t* ds, BattleEvent event, uint16_t element);

// The services of this module.
uint16_t svc_btladv(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count);
