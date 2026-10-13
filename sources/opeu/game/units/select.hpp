// Selection recovery for divisions, units and elements.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_select_first_group(Game& game, uint8_t side);
void opeu_select_first_unit(Game& game, uint16_t group);
void opeu_select_preferred_element(Game& game, uint16_t unit);
