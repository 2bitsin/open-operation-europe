#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_map_inspect(uint8_t* ds, HostEvent event, uint8_t x, uint8_t y);

// The services of this module.
uint16_t svc_inspect(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count);
