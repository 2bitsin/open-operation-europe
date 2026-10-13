#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_overview_unit_draw(Game& game, HostEvent event, uint16_t unit, uint16_t x, uint16_t y);
void opeu_overview_cell_draw(Game& game, HostEvent event, uint8_t x, uint8_t y);
