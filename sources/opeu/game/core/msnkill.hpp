// The special-forces assassination mission and the service table of the mission routines.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_mission_assassinate(uint8_t* ds, HostEvent event, uint8_t side);

// The services of the mission routines (MissionService).
uint16_t svc_mission(const char* module, uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
uint16_t svc_msnkill(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
