// The Airlift command: the cargo amounts, drop point and delay of a supply airlift.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_air_lift_lead(uint8_t* ds, HostEvent event, uint16_t sp, uint8_t hours, uint16_t planes);
uint16_t opeu_air_lift_order(uint8_t* ds, HostEvent event, uint16_t sp, uint8_t x, uint8_t y, uint8_t hours, uint16_t planes, uint16_t ammo, uint16_t fuel, uint16_t food);
void     opeu_air_lift_command(uint8_t* ds, HostEvent event, uint16_t sp);

// Runs the services the module's routines call through their event.
uint16_t svc_airlift(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
