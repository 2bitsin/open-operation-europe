// The hourly special-forces mission countdown and the first hourly turn step.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_mission_target_valid(Game& game, uint16_t mission, uint8_t side);
void     opeu_mission_tick(Game& game, HostEvent event);
void     opeu_turn_hourly_a(Game& game, HostEvent event);

// The services of the mission routines (MissionService).
uint16_t svc_msntick(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
