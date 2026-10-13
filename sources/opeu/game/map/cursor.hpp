#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_map_cursor(uint8_t* ds, HostEvent event, uint16_t x, uint16_t y);

// The services of this module.
uint16_t svc_cursor(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
