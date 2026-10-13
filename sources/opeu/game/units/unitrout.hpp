// The hourly morale checks of the regiments: falling back and opportunistic attacks.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_unit_fall_back(Game& game, uint16_t unit, uint8_t x, uint8_t y);
void opeu_unit_morale_check(Game& game, uint8_t side, uint8_t group, uint8_t index);
