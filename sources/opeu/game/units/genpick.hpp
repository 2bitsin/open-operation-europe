// The general-list callbacks of the staff and division assignment screen.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_general_assign_choice(Game& game, uint8_t choice);
void    opeu_general_list_draw_rows(Game& game, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags);
