#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_control_position(Game& game, uint16_t x, uint16_t y);
void opeu_map_center(Game& game, uint8_t x, uint8_t y, uint8_t side);
void opeu_arrival_location(Game& game, uint8_t x, uint8_t y, uint8_t side);
