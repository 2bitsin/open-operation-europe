#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_view_cell_clear(Game& game, int16_t x, int16_t y);
void opeu_view_redraw(Game& game);
void opeu_view_scroll(Game& game, int16_t x, int16_t y);
void opeu_view_scroll_to(Game& game, int16_t x, int16_t y);
void opeu_map_scroll_to(Game& game, int16_t x, int16_t y, uint8_t side);
