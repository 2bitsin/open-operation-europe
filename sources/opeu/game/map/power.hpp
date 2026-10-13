#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_item_strength(Game const& game, uint16_t item);
int16_t  opeu_unit_strength(Game const& game, uint16_t unit);
int16_t  opeu_objective_strength(Game const& game, uint16_t objective);
void     opeu_add_influence(int16_t amount, int16_t* value);
int16_t  opeu_weight_influence(uint8_t x, uint8_t y, uint8_t x2, uint8_t y2, int16_t value);
