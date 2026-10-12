// Resources and capacities of groups, sides and objectives.
#pragma once

#include "types.hpp"

void opeu_group_resources(uint8_t* ds, uint16_t group, uint32_t* result_a, uint32_t* result_b, uint32_t* result_c,
                          uint16_t stored);
void opeu_side_resources(uint8_t* ds, uint8_t side, uint32_t* result_a, uint32_t* result_b, uint32_t* result_c,
                         uint16_t stored);
void opeu_objective_capacities(uint8_t* ds, uint16_t objective, uint16_t* result_a, uint16_t* result_b,
                               uint16_t* result_c);
