// A unit's resources as percentages of its carrying capacity.
#pragma once

#include "types.hpp"

void opeu_unit_resource_percentages(const uint8_t* ds, uint16_t unit, uint16_t* result_a, uint16_t* result_b,
                                    uint16_t* result_c);
