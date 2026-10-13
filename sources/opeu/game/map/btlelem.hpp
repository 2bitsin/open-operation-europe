#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_draw_element_at(Game& game, uint8_t x, uint8_t y, uint16_t element, uint8_t mode);
void opeu_battle_redraw_cell(Game& game, uint8_t x, uint8_t y, uint16_t element, uint8_t mode);
void opeu_battle_refresh_cell(Game& game, uint8_t x, uint8_t y);
void opeu_battle_draw_element_frame(Game& game, uint8_t x, uint8_t y, uint16_t offset, uint8_t frame_index, uint16_t element);
void opeu_battle_animate_element(Game& game, uint8_t x, uint8_t y, uint16_t turn, uint16_t pause, uint16_t offset, uint16_t element);
