// The best terrain rating of a unit at a map cell.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_best_terrain(Game const& game, uint8_t x, uint8_t y, uint16_t unit);
