// The map inspection mode: a cursor over the map, its panels and the regiment orders it leads to.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_control_coordinates(Game& game, HostEvent event, uint16_t x, uint16_t y);

// Runs the services the module's routines call through their event.
uint16_t svc_coords(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
