#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_battle_dispatch(Game& game, uint8_t handler);
uint16_t opeu_battle_cell_row(Game& game, uint8_t x, uint8_t y);
uint16_t opeu_battle_cell_column(Game& game, uint8_t x, uint8_t y);
void     opeu_battle_draw_cursor(Game& game, uint8_t x, uint8_t y);
void     opeu_battle_draw_frame(Game& game, uint8_t x, uint8_t y);
void     opeu_battle_draw_number(Game& game, uint16_t value);
