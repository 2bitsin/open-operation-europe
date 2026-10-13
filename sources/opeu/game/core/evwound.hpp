// Hourly wound recovery of the generals, and the death of a general replaced by a deputy.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_death_present(Game& game, HostEvent event);
void opeu_commander_death(Game& game, HostEvent event, uint16_t person);
void opeu_wound_event(Game& game, HostEvent event);

// Runs the services the module's routines call through their event.
uint16_t svc_evwound(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
