// The hourly special-forces mission countdown and the first hourly turn step.
#pragma once

#include "types.hpp"
#include "types/events.hpp"

uint16_t opeu_mission_target_valid(uint8_t* ds, uint16_t mission, uint8_t side);
void     opeu_mission_tick(uint8_t* ds, HostEvent event);
void     opeu_turn_hourly_a(uint8_t* ds, HostEvent event);

// The services of the mission routines (MissionService).
uint16_t svc_msntick(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count);
