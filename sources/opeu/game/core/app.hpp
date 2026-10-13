// The game's main loop over the calendar and the turn order.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_application_entry(Game& game, uint16_t (*event)(Game&, uint16_t));
void opeu_select_turn_context(Game& game);
void opeu_begin_hour(Game& game);

// Runs the services the main loop calls through its event.
uint16_t svc_app(Game& game, uint16_t id);
