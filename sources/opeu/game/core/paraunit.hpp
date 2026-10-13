// The paratroop regiment: forming it, judging landing cells, and landing it.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_para_unit_create(Game& game, uint8_t side, uint8_t x, uint8_t y);
uint16_t opeu_para_cell_ok(Game& game, uint8_t x, uint8_t y);
uint16_t opeu_para_scatter_check(Game& game);
uint16_t opeu_para_land(Game& game, uint16_t unit, uint16_t losses);
