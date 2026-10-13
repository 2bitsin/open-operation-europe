// Hourly wound recovery of the generals, and the death of a general replaced by a deputy.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_death_present(Game& game);
void opeu_commander_death(Game& game, uint16_t person);
void opeu_wound_event(Game& game);
