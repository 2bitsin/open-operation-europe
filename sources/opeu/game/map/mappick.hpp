#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_map_pick_location(Game& game, HostEvent event, uint16_t x, uint16_t y);

// The services of this module.
uint16_t svc_mappick(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
