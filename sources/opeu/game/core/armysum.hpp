// The army summary of the current side's panel.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_army_summary(uint8_t* ds, HostEvent event, uint16_t scratch);

// Runs the services the module's routines call through their event.
uint16_t svc_armysum(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count);
