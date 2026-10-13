// The Bombing Attack command, and svc_air, the services of the air force's commands.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_air_bomb_check(uint8_t* ds, HostEvent event, uint8_t city, uint16_t type);
uint16_t opeu_air_bomb_order(uint8_t* ds, HostEvent event, uint8_t x, uint8_t y, uint16_t type);
void     opeu_air_bomb_command(uint8_t* ds, HostEvent event);

// Runs the services the air force's commands call through their event (common/air-service.hpp); module names the
// caller in the report of an unknown service.
uint16_t svc_air(uint8_t* ds, const char* module, uint16_t id, const uint16_t* args, uint16_t count);
// Runs the services the Bombing Attack command calls through its event.
uint16_t svc_airbomb(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
