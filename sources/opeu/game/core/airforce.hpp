// The Air Force command: the air-force officer and the mission table.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_air_officer(Game& game);
void opeu_air_header(Game& game, HostEvent event);
void opeu_turn_action_8(Game& game, HostEvent event);

// Runs the services the module's routines call through their event.
uint16_t svc_airforce(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
