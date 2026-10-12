// The turn of the current regiment: the scheduled daily, half-day and hourly rounds, then its side's control.
#pragma once

#include "types.hpp"

void opeu_process_selected_turn(uint8_t* ds, uint16_t (*event)(uint8_t*, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t));

// Runs the services the module's routines call through their event.
uint16_t svc_dispatch(uint8_t* ds, uint16_t id, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3);
