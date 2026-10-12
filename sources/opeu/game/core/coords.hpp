// The map inspection mode: a cursor over the map, its panels and the regiment orders it leads to.
#pragma once

#include "types.hpp"
#include "types/events.hpp"

uint16_t opeu_control_coordinates(uint8_t* ds, HostEvent event,
                                  uint16_t scratch, uint16_t x, uint16_t y);

// Runs the services the module's routines call through their event.
uint16_t svc_coords(uint8_t* ds, uint16_t id, const uint16_t* args,
                    uint16_t count);
