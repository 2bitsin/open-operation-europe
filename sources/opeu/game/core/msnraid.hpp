// The special-forces raids, sabotage and cuts of communications.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_mission_raid_objective(uint8_t* ds, HostEvent event, uint16_t frame, uint8_t side);
void opeu_mission_raid_unit(uint8_t* ds, HostEvent event, uint16_t frame, uint8_t side);
void opeu_mission_sabotage(uint8_t* ds, HostEvent event, uint16_t frame, uint8_t side);
void opeu_mission_objective_3(uint8_t* ds, HostEvent event, uint16_t frame, uint8_t side);
void opeu_mission_unit_4(uint8_t* ds, HostEvent event, uint16_t frame, uint8_t side);

// The services of the mission routines (MissionService).
uint16_t svc_msnraid(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
