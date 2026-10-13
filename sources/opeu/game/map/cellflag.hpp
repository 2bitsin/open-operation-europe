#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_mark_cell(Game& game, uint8_t x, uint8_t y, uint8_t side);
void     opeu_unmark_cell(Game& game, uint8_t x, uint8_t y, uint8_t side);
uint16_t opeu_cell_marked(Game& game, uint8_t x, uint8_t y, uint8_t side);
int16_t  opeu_cell_influence(Game& game, uint8_t x, uint8_t y);
uint8_t  opeu_cell_influence_side(Game& game, uint8_t x, uint8_t y);
