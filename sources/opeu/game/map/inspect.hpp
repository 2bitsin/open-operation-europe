#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_map_inspect(Game& game, HostEvent event, uint8_t x, uint8_t y);

// The services of this module.
uint16_t svc_inspect(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
