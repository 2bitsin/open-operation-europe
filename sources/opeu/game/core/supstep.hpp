// The interactive supply allocation between the two columns of the supply table.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

uint16_t opeu_supply_step(uint8_t* ds, HostEvent event, uint16_t selection);

// The services of the supply allocation as direct calls (the panel's input services through core/suppanel).
uint16_t svc_supstep(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count);
