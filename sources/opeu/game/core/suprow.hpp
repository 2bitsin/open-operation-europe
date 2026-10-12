// Redraws one row of the supply-allocation panel.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_supply_draw_row(uint8_t* ds, HostEvent event, uint8_t row,
                          uint8_t column);

// The text, glyph and fill services of the row redraw as direct calls.
uint16_t svc_suprow(uint8_t* ds, uint16_t id, const uint16_t* args,
                    uint16_t count);
