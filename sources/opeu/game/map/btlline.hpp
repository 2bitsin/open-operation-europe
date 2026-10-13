#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_draw_line(Game& game, BattleEvent event, int16_t x0, int16_t y0, int16_t x1, int16_t y1);

// The services of this module.
uint16_t svc_btlline(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
