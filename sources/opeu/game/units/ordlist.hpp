// Orders command (turn action 1).
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_order_units_fill(Game& game, uint8_t group);
void    opeu_turn_action_1(Game& game, uint16_t segment);
