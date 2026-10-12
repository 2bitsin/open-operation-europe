// The Air Force command: the air-force officer and the mission table.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_air_officer(uint8_t* ds, HostEvent event, uint16_t sp);
void opeu_air_header(uint8_t* ds, HostEvent event, uint16_t sp);
void opeu_turn_action_8(uint8_t* ds, HostEvent event, uint16_t sp);

// Runs the services the module's routines call through their event.
uint16_t svc_airforce(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
