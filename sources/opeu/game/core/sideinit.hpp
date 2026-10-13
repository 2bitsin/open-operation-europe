// The start of a side's turn: the scenario's side initializer for a side a player commands.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_initialize_turn_side(Game& game, uint16_t (*event)(Game&, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t));

// The services opeu_initialize_turn_side calls.
uint16_t svc_sideinit(Game& game, uint16_t id, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3);
