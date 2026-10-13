// The panels of the scenario-selection screen.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_scenario_frame(Game& game, HostEvent event);
void opeu_scenario_message(Game& game, HostEvent event, uint16_t text);
void opeu_scenario_clear_panel(Game& game, HostEvent event);
void opeu_scenario_clear_text(Game& game, HostEvent event);
void opeu_scenario_description(Game& game, HostEvent event, uint16_t text);

// The services of this module in the native build.
uint16_t svc_scenui(Game& game, uint16_t kind, const uint16_t* args, uint16_t count);
