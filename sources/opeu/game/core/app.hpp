// The game's main loop over the calendar and the turn order.
#pragma once

#include "types.hpp"

void opeu_application_entry(uint8_t* ds, uint16_t (*event)(uint8_t*, uint16_t));
void opeu_select_turn_context(uint8_t* ds);
void opeu_begin_hour(uint8_t* ds);

// Runs the services the main loop calls through its event.
uint16_t svc_app(uint8_t* ds, uint16_t id);
