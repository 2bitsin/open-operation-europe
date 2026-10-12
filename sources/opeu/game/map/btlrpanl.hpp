#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_wiped_notice(uint8_t* ds, BattleEvent event, uint16_t element, uint16_t frame);
void opeu_battle_ranged_panel(uint8_t* ds, BattleEvent event, uint16_t shooter, uint16_t target, uint16_t frame);

// The services of this module.
uint16_t svc_btlrpanl(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count);
