// The start of a side's turn: the scenario's side initializer for a side a player commands.
#pragma once

#include "types.hpp"

void opeu_initialize_turn_side(uint8_t* ds,
                               uint16_t (*event)(uint8_t*, uint16_t, uint16_t,
                                                 uint16_t, uint16_t, uint16_t));

// The services opeu_initialize_turn_side calls.
uint16_t svc_sideinit(uint8_t* ds, uint16_t id, uint16_t a0, uint16_t a1,
                      uint16_t a2, uint16_t a3);
