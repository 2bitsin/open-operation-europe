// The turn of the current regiment: the scheduled daily, half-day and hourly rounds, then its side's control.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_process_selected_turn(Game& game, uint16_t (*event)(Game&, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t));

// Runs the services the module's routines call through their event.
uint16_t svc_dispatch(Game& game, uint16_t id, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3);
