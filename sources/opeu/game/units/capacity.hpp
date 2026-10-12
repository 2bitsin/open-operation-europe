// The resource capacities of a unit.
#pragma once

#include "types.hpp"

void opeu_unit_capacities(const uint8_t* ds, uint16_t unit, uint16_t* result_a,
                          uint16_t* result_b, uint16_t* result_c);
