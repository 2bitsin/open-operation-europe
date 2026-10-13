// Row-draw callbacks of a list table (see the .cpp).
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_tbl_draw_units(Game& game, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags);
void opeu_tbl_draw_orders(Game& game, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags);
