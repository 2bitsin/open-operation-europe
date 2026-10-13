// Side helpers: the other side, the side a status points to, the side colour, the side an equipment type serves.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_opposite_side(uint8_t side);
uint8_t opeu_side_for_status(Game const& game, uint8_t side);
void    opeu_select_side_color(Game& game, uint8_t side);
uint8_t opeu_record_side(Game const& game, uint16_t record);
