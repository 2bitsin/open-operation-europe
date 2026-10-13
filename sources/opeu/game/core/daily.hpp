// Dated events: each side's events of the current month, at the start of a day.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_scan_daily_events(Game& game);
void opeu_turn_daily(Game& game);
