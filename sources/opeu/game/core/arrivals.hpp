// The arrival of reserve units.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_process_arrivals(Game& game, uint8_t side);
void opeu_hourly_c_stage_c(Game& game);
