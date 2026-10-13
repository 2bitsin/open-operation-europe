#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_unit_flash(Game& game, uint16_t record, uint8_t ticks);
void opeu_unit_announce(Game& game, uint16_t unit);
void opeu_group_show(Game& game, uint16_t group, uint8_t person);
void opeu_arrival_view(Game& game, uint16_t unit);
void opeu_side_cell_flash(Game& game, uint8_t x, uint8_t y, uint8_t side);
void opeu_location_flash(Game& game, uint8_t x, uint8_t y, uint8_t side);
