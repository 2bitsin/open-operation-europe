// Equipment type names, nation codes, chain length and the unit-slot class comparison.
#pragma once

#include "common/events.hpp"
#include "common/text.hpp"
#include "types.hpp"

TextTarget opeu_item_type_name(Game& game, uint16_t item, TextTarget buffer);
TextTarget opeu_nation_code(Game& game, uint16_t type, TextTarget buffer);
uint8_t    opeu_list_length(Game& game, uint16_t list);
uint16_t   opeu_same_unit_class(uint16_t first, uint16_t second);
