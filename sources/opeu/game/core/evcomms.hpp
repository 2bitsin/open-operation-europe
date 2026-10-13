// Hourly objective events: strength growth, restored communications and caught spies.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_objective_comms_event(Game& game, HostEvent event);

// Runs the services the module's routines call through their event.
uint16_t svc_evcomms(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
