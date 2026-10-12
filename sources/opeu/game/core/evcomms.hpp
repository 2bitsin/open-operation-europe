// Hourly objective events: strength growth, restored communications and caught spies.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_objective_comms_event(uint8_t* ds, HostEvent event, uint16_t sp);

// Runs the services the module's routines call through their event.
uint16_t svc_evcomms(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
