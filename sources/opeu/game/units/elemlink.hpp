// Linking elements into units and unlinking them.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void    opeu_insert_element(Game& game, uint16_t unit, uint16_t element);
uint8_t opeu_remove_element(Game& game, uint16_t unit, uint16_t element);
