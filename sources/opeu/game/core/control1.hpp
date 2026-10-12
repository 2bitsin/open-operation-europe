// Turn control of a side the player commands: its command menu and the map inspection mode.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_turn_control_one(uint8_t* ds, HostEvent event,
                           HostEvent context_event, uint16_t scratch);

// Runs the services the module's routines call through their event.
uint16_t svc_control1(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count);
