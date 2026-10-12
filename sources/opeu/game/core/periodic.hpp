// The state changes of the day's turn points: generals' counters and the sides' pools.
#pragma once

#include "types.hpp"

void opeu_decrement_record_counters(uint8_t* ds);
void opeu_apply_daily_side_resources(uint8_t* ds);
