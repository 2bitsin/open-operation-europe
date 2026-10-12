#pragma once

#include "types.hpp"
#include "types/events.hpp"

void opeu_control_position(uint8_t* ds, uint16_t x, uint16_t y);
void opeu_map_center(uint8_t* ds, HostEvent event, uint16_t frame, uint8_t x,
                     uint8_t y, uint8_t side);
void opeu_arrival_location(uint8_t* ds, HostEvent event, uint16_t frame,
                           uint8_t x, uint8_t y, uint8_t side);

// The services of this module.
uint16_t svc_mappos(uint8_t* ds, uint16_t kind, const uint16_t* args,
                    uint16_t count);
