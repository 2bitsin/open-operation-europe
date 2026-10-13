// The hourly handling of the sides' requests: a booked request is carried out when its delay runs out, and each hour a
// filed request may reach headquarters, which accepts or turns it down.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_hourly_c_stage_b(Game& game, uint16_t stale_threshold);
