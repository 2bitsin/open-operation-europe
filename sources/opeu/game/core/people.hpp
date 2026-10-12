// Finding a side's general by staff role or by division.
#pragma once

#include "types.hpp"

uint16_t opeu_find_side_record(const uint8_t* ds, uint8_t side, uint8_t kind);
uint16_t opeu_find_group_record(const uint8_t* ds, uint8_t side, uint8_t group);
