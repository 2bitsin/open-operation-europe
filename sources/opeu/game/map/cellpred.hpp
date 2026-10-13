#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_request_cell_allowed(Game& game, uint8_t x, uint8_t y, uint8_t side);
uint16_t opeu_arrival_cell_allowed(Game& game, uint8_t x, uint8_t y, uint8_t side);
