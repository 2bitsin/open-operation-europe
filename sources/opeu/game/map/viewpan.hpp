#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_viewport_pan(uint8_t* ds, HostEvent event, int16_t dx, int16_t dy,
                       uint8_t x, uint8_t y, uint8_t side);

// The services of this module.
uint16_t svc_viewpan(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count);
