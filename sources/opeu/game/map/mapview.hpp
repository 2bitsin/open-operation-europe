#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_view_unit_draw(uint8_t* ds, HostEvent event, uint16_t frame, uint16_t unit, uint16_t x, uint16_t y);
void opeu_view_cell_draw(uint8_t* ds, HostEvent event, uint16_t frame, uint16_t x, uint16_t y);

// The services of this module.
uint16_t svc_mapview(uint8_t* ds, uint16_t kind, const uint16_t* args, uint16_t count);
