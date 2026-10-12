// A resource amount as a percentage of a capacity, capped at 999.
#pragma once

#include "types.hpp"

uint16_t opeu_resource_percent_wide(int32_t current, int32_t capacity);
uint16_t opeu_resource_percent(uint16_t current, uint16_t capacity);
