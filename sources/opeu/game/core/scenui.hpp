// The panels of the scenario-selection screen.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_scenario_frame(uint8_t* ds, HostEvent event);
void opeu_scenario_message(uint8_t* ds, HostEvent event, uint16_t text);
void opeu_scenario_clear_panel(uint8_t* ds, HostEvent event);
void opeu_scenario_clear_text(uint8_t* ds, HostEvent event);
void opeu_scenario_description(uint8_t* ds, HostEvent event, uint16_t text);

// The services of this module in the native build.
uint16_t svc_scenui(uint8_t* ds, uint16_t kind, const uint16_t* args,
                    uint16_t count);
