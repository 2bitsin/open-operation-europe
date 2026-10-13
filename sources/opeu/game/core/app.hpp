// The game's main loop over the calendar and the turn order.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_application_entry(Game& game);
void opeu_select_turn_context(Game& game);
void opeu_begin_hour(Game& game);
