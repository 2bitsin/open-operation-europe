// Redraws one row of the supply-allocation panel.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_supply_draw_row(Game& game, HostEvent event, uint8_t row, uint8_t column);

// The text, glyph and fill services of the row redraw as direct calls.
uint16_t svc_suprow(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
