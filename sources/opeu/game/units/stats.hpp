// Element averages and quantity totals of units, divisions, sides and objectives.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_unit_special(Game& game, uint16_t unit);
uint16_t opeu_unit_average9(Game& game, uint16_t unit);
uint16_t opeu_unit_average10(Game& game, uint16_t unit);
uint16_t opeu_unit_average11(Game& game, uint16_t unit);
uint16_t opeu_unit_quantity(Game& game, uint16_t unit, uint8_t category, uint8_t* count);
uint16_t opeu_group_quantity(Game& game, uint16_t group, uint8_t category, uint8_t* count);
uint32_t opeu_side_quantity(Game& game, uint8_t side, uint8_t category, uint8_t* count);
uint16_t opeu_objective_quantity(Game& game, uint16_t objective, uint8_t category, uint8_t* count);
