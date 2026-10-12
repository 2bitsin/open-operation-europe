#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_battle_draw_line(uint8_t* ds, BattleEvent event, int16_t x0,
                           int16_t y0, int16_t x1, int16_t y1);

// The services of this module.
uint16_t svc_btlline(uint8_t* ds, uint32_t address, const uint16_t* args,
                     unsigned int count);
