#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_unit_flash(uint8_t* ds, HostEvent event, uint16_t record, uint8_t ticks);
void opeu_unit_announce(uint8_t* ds, HostEvent event, uint16_t unit);
void opeu_group_show(uint8_t* ds, HostEvent event, uint16_t group, uint8_t person);
void opeu_arrival_view(uint8_t* ds, HostEvent event, uint16_t unit);
void opeu_side_cell_flash(uint8_t* ds, HostEvent event, uint8_t x, uint8_t y, uint8_t side);
void opeu_location_flash(uint8_t* ds, HostEvent event, uint8_t x, uint8_t y, uint8_t side);

// The services of this module.
uint16_t svc_mapunit(uint8_t* ds, uint16_t kind, const uint16_t* args, uint16_t count);
