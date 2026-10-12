#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_commander_lost(uint8_t* ds, BattleEvent event);
void opeu_battle_report_loss(uint8_t* ds, BattleEvent event);
void opeu_battle_element_lost(uint8_t* ds, BattleEvent event, uint16_t element);

// The services of this module.
uint16_t svc_btldepty(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count);
