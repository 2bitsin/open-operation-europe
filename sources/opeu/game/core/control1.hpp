// Turn control of a side the player commands: its command menu and the map inspection mode.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_turn_control_one(Game& game, HostEvent event);

// Runs the services the module's routines call through their event.
uint16_t svc_control1(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
