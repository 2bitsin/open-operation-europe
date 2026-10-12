#pragma once

#include "types.hpp"

void    opeu_place_unit(uint8_t* ds, uint16_t unit);
void    opeu_remove_unit(uint8_t* ds, uint16_t unit);
uint8_t opeu_cell_side(const uint8_t* ds, uint8_t x, uint8_t y);
