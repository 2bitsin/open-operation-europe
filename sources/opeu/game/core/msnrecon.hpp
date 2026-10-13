// The special-forces reconnaissance missions.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_mission_recon_objective(uint8_t* ds, HostEvent event, uint8_t side);
void opeu_mission_recon_unit(uint8_t* ds, HostEvent event, uint8_t side);

// The services of the mission routines (MissionService).
uint16_t svc_msnrecon(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
