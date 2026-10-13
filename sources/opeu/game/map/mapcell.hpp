#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_view_cell_paint(uint8_t* ds, HostEvent event, uint16_t x, uint16_t y, uint16_t mode);
void opeu_map_cell_flash(uint8_t* ds, HostEvent event, uint8_t x, uint8_t y);

// The services of this module.
uint16_t svc_mapcell(uint8_t* ds, uint16_t kind, const uint16_t* args, uint16_t count);
