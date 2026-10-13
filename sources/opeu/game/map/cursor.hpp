#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_map_cursor(Game& game, HostEvent event, uint16_t x, uint16_t y);

// The services of this module.
uint16_t svc_cursor(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
