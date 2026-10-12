#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_battle_draw_cell(uint8_t* ds, BattleEvent event, uint8_t x,
                           uint8_t y);
void opeu_battle_draw_cell_above(uint8_t* ds, BattleEvent event, uint8_t x,
                                 uint8_t y);

// The services of this module.
uint16_t svc_btlcell(uint8_t* ds, uint32_t address, const uint16_t* args,
                     unsigned int count);
