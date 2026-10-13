// The division and mission tables' row-draw callbacks and the service table of every row-draw callback.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_tbl_draw_groups(Game& game, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags);
void opeu_tbl_draw_missions(Game& game, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags);
