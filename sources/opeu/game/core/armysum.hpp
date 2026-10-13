// The army summary of the current side's panel.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_army_summary(Game& game, HostEvent event);

// Runs the services the module's routines call through their event.
uint16_t svc_armysum(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
