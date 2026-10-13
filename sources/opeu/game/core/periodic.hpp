// The state changes of the day's turn points: generals' counters and the sides' pools.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_decrement_record_counters(Game& game);
void opeu_apply_daily_side_resources(Game& game);
