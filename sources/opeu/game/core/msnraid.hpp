// The special-forces raids, sabotage and cuts of communications.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_mission_raid_objective(Game& game, HostEvent event, uint8_t side);
void opeu_mission_raid_unit(Game& game, HostEvent event, uint8_t side);
void opeu_mission_sabotage(Game& game, HostEvent event, uint8_t side);
void opeu_mission_objective_3(Game& game, HostEvent event, uint8_t side);
void opeu_mission_unit_4(Game& game, HostEvent event, uint8_t side);

// The services of the mission routines (MissionService).
uint16_t svc_msnraid(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
