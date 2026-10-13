#pragma once

#include "common/events.hpp"
#include "types.hpp"

void    opeu_place_unit(Game& game, uint16_t unit);
void    opeu_remove_unit(Game& game, uint16_t unit);
uint8_t opeu_cell_side(Game const& game, uint8_t x, uint8_t y);
