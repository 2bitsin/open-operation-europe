// An objective's resources as percentages of its storage capacity.
#pragma once

#include "types.hpp"

void opeu_objective_resource_percentages(uint8_t* ds, uint16_t objective, uint16_t* result_a, uint16_t* result_b,
                                         uint16_t* result_c);
