// The hourly morale checks of the regiments: falling back and opportunistic attacks.
#pragma once

#include "types.hpp"

void opeu_unit_fall_back(uint8_t* ds, uint16_t unit, uint8_t x, uint8_t y);
void opeu_unit_morale_check(uint8_t* ds, uint8_t side, uint8_t group, uint8_t index);
