// The hourly condition of every ready regiment: communications, spies, food, fatigue and morale.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_unit_hourly_event(uint8_t* ds, HostEvent event);

// Runs the services the module's routines call through their event.
uint16_t svc_evunits(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
