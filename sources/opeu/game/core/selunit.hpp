// The selected unit of a division or of a side.
#pragma once

#include "types.hpp"

uint16_t opeu_find_group_selected_unit(const uint8_t* ds, uint16_t group);
uint16_t opeu_find_side_selected_unit(const uint8_t* ds, uint8_t side);
