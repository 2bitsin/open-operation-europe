#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_map_erase(uint8_t* ds, HostEvent event, uint8_t x, uint8_t y);
void opeu_arrival_prepare(uint8_t* ds, HostEvent event, uint16_t unit);
void opeu_unit_remove_draw(uint8_t* ds, HostEvent event, uint16_t unit);
void opeu_daily_activate(uint8_t* ds, HostEvent event, uint8_t side);

// The services of this module.
uint16_t svc_maperase(uint8_t* ds, uint16_t kind, const uint16_t* args, uint16_t count);
