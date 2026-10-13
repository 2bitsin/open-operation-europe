// Equipment type names, nation codes, chain length and the unit-slot class comparison.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_item_type_name(Game& game, uint16_t item, uint16_t buffer);
uint16_t opeu_nation_code(Game& game, uint16_t type, uint16_t buffer);
uint8_t  opeu_list_length(Game& game, uint16_t list);
uint16_t opeu_same_unit_class(uint16_t first, uint16_t second);
