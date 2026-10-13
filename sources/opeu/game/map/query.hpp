#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_find_unit_at(Game const& game, uint8_t x, uint8_t y);
uint16_t opeu_find_objective_at(Game const& game, uint8_t x, uint8_t y);
uint8_t  opeu_objective_side(Game const& game, uint16_t objective);
