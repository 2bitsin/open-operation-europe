// Linking elements into units and unlinking them.
#pragma once

#include "types.hpp"

void    opeu_insert_element(uint8_t* ds, uint16_t unit, uint16_t element);
uint8_t opeu_remove_element(uint8_t* ds, uint16_t unit, uint16_t element);
