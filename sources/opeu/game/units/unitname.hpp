// The name and number of a regiment.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_unit_name(uint8_t* ds, HostEvent event, uint16_t unit, uint16_t buffer);

// The services the module calls out to.
uint16_t svc_unitname(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
