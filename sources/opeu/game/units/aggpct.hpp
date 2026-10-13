// Group and side resource percentages.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_group_resource_percentages(Game& game, uint16_t group, uint16_t* result_a, uint16_t* result_b, uint16_t* result_c);
void opeu_side_resource_percentages(Game& game, uint8_t side, uint16_t* result_a, uint16_t* result_b, uint16_t* result_c, uint16_t units_only);
