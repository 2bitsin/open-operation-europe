#pragma once

#include "types.hpp"

void     opeu_overview_pixel(uint8_t* ds, uint16_t x, uint16_t y);
void     opeu_view_pixel(uint8_t* ds, uint16_t x, uint16_t y);
void     opeu_sprite_recolor(uint8_t* ds, uint16_t data, uint16_t size,
                             uint8_t side);
uint16_t opeu_unit_visible(uint8_t* ds, uint16_t unit);
