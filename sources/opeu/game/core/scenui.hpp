// The panels of the scenario-selection screen.
#pragma once

#include "common/events.hpp"
#include "common/text.hpp"
#include "types.hpp"

void opeu_scenario_frame(Game& game);
void opeu_scenario_message(Game& game, uint16_t text);
void opeu_scenario_clear_panel(Game& game);
void opeu_scenario_clear_text(Game& game);
void opeu_scenario_description(Game& game, Text text);
