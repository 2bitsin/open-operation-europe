#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_cell_open(Game& game, uint8_t x, uint8_t y, uint16_t element);
void     opeu_battle_cursor_draw(Game& game);
void     opeu_battle_cursor_step(Game& game, int16_t dx, int16_t dy);
void     opeu_battle_mouse_cell(Game& game, uint16_t x_at, uint16_t y_at);
