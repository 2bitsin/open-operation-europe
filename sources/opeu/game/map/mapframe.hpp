#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_view_number(Game& game, uint16_t x, uint16_t y, uint16_t value);
void opeu_scale_label_x(Game& game, uint16_t x, uint16_t y, uint16_t back, uint16_t fore, int16_t value);
void opeu_scale_label_y(Game& game, uint16_t x, uint16_t y, uint16_t back, uint16_t fore, int16_t value);
void opeu_overview_frame(Game& game);
void opeu_view_frame(Game& game, int16_t x, int16_t y);
