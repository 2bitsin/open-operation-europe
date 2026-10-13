#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_move_adjacent(Game& game, uint8_t x, uint8_t y);
uint16_t opeu_move_objective_target(Game& game, uint16_t x, uint16_t y, uint8_t side);
uint16_t opeu_move_unit_target(Game& game, uint16_t x, uint16_t y);
uint16_t opeu_move_level_raise(Game& game, uint16_t record, uint8_t value);
uint16_t opeu_move_level_lower(Game& game, uint16_t record, uint8_t value);
