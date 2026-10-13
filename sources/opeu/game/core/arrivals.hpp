// The arrival of reserve units.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

// event(game, id, a0, a1, a2, a3): the services of the arrivals (svc_arrivals).
void opeu_process_arrivals(Game& game, uint8_t side);
void opeu_hourly_c_stage_c(Game& game);
