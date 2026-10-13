#pragma once

#include "common/data-segment.hpp"
#include "common/map-frame.hpp"
#include "types.hpp"

// The overview's and the tactical view's pixel position of a map cell (x counts 8-pixel byte columns).
[[nodiscard]] auto OverviewPixel(opeu::game::Pixel cell) -> opeu::game::Pixel;
[[nodiscard]] auto ViewPixel(opeu::game::DataSegment const& data, opeu::game::Pixel cell) -> opeu::game::Pixel;

void     opeu_overview_pixel(uint8_t* ds, uint16_t x, uint16_t y);
void     opeu_view_pixel(uint8_t* ds, uint16_t x, uint16_t y);
void     opeu_sprite_recolor(uint8_t* ds, uint16_t data, uint16_t size, uint8_t side);
uint16_t opeu_unit_visible(uint8_t* ds, uint16_t unit);
