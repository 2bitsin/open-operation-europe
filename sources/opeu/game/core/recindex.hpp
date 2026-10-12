// Where a record sits in its table: the side, division and index of a division, unit or element.
#pragma once

#include "types.hpp"

uint8_t  opeu_group_index(uint16_t group);
uint8_t  opeu_group_side(uint16_t group);
uint8_t  opeu_unit_index(uint16_t unit);
uint8_t  opeu_unit_group_index(uint16_t unit);
uint8_t  opeu_unit_side(uint16_t unit);
uint16_t opeu_unit_group(uint16_t unit);
uint8_t  opeu_element_side(uint16_t element);
