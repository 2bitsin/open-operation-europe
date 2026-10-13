#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_control_position(Game& game, uint16_t x, uint16_t y);
void opeu_map_center(Game& game, HostEvent event, uint8_t x, uint8_t y, uint8_t side);
void opeu_arrival_location(Game& game, HostEvent event, uint8_t x, uint8_t y, uint8_t side);

// The services of this module.
uint16_t svc_mappos(Game& game, uint16_t kind, const uint16_t* args, uint16_t count);
