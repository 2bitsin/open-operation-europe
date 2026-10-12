// Hourly wound recovery of the generals, and the death of a general replaced by a deputy.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_death_present(uint8_t* ds, HostEvent event);
void opeu_commander_death(uint8_t* ds, HostEvent event, uint16_t bp, uint16_t person);
void opeu_wound_event(uint8_t* ds, HostEvent event, uint16_t sp);

// Runs the services the module's routines call through their event.
uint16_t svc_evwound(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
