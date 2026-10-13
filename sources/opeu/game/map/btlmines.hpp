#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_battle_mine_report(uint8_t* ds, BattleEvent event, uint16_t element);
uint16_t opeu_battle_mine_damage(uint8_t* ds, BattleEvent event, uint16_t element);
void     opeu_battle_mine_hit(uint8_t* ds, BattleEvent event, uint16_t element);

// The services of this module.
uint16_t svc_battle_mines(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned count);
uint16_t svc_btlmines(uint8_t* ds, uint32_t service, const uint16_t* args, unsigned int count);
