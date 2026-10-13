// The special-forces assassination mission and the service table of the mission routines.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_mission_assassinate(Game& game, HostEvent event, uint8_t side);

// The services of the mission routines (MissionService).
uint16_t svc_mission(const char* module, Game& game, uint16_t id, const uint16_t* args, uint16_t count);
uint16_t svc_msnkill(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
