// Dated events: each side's events of the current month, at the start of a day.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_scan_daily_events(Game& game, uint16_t (*event)(Game&, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t));
void opeu_turn_daily(Game& game, uint16_t (*event)(Game&, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t));

// The services of this module as direct calls.
uint16_t svc_daily(Game& game, uint16_t id, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3);
