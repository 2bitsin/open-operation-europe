#pragma once

#include "types.hpp"

void     opeu_mark_cell(uint8_t* ds, uint8_t x, uint8_t y, uint8_t side);
void     opeu_unmark_cell(uint8_t* ds, uint8_t x, uint8_t y, uint8_t side);
uint16_t opeu_cell_marked(uint8_t* ds, uint8_t x, uint8_t y, uint8_t side);
int16_t  opeu_cell_influence(uint8_t* ds, uint8_t x, uint8_t y);
uint8_t  opeu_cell_influence_side(uint8_t* ds, uint8_t x, uint8_t y);
