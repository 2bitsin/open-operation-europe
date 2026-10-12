// Dated events: each side's events of the current month, at the start of a day.
#pragma once

#include "types.hpp"

void opeu_scan_daily_events(uint8_t* ds, uint16_t (*event)(uint8_t*, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t));
void opeu_turn_daily(uint8_t* ds, uint16_t (*event)(uint8_t*, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t));

// The services of this module as direct calls.
uint16_t svc_daily(uint8_t* ds, uint16_t id, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3);
