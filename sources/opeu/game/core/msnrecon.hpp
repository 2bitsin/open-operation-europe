// The special-forces reconnaissance missions.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_mission_recon_objective(Game& game, HostEvent event, uint8_t side);
void opeu_mission_recon_unit(Game& game, HostEvent event, uint8_t side);

// The services of the mission routines (MissionService).
uint16_t svc_msnrecon(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
