#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_draw_cell(Game& game, BattleEvent event, uint8_t x, uint8_t y);
void opeu_battle_draw_cell_above(Game& game, BattleEvent event, uint8_t x, uint8_t y);

// The services of this module.
uint16_t svc_btlcell(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
