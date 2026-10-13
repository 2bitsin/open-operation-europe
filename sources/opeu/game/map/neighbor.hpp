#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_side_supported_units(Game& game, uint8_t side, uint16_t out);
uint8_t opeu_neighbor_units(Game& game, uint16_t unit, uint16_t out);
