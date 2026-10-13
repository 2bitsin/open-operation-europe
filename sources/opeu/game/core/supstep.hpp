// The interactive supply allocation between the two columns of the supply table.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_supply_step(uint8_t* ds, HostEvent event);

// The services of the supply allocation as direct calls (the panel's input services through core/suppanel).
uint16_t svc_supstep(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
