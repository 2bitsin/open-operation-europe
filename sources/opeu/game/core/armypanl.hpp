// The division panel's regiments, their slot grids and element rows.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_panel_slots(Game& game, HostEvent event, int y, uint8_t count);
uint32_t opeu_element_icon(Game const& game, uint16_t element);
void     opeu_panel_elements(Game& game, HostEvent event, uint16_t unit, int y);
void     opeu_panel_battle_frame(Game& game, HostEvent event, int y);
void     opeu_division_regiments(Game& game, HostEvent event, uint16_t units, uint16_t person);

// Runs the services the module's routines call through their event.
uint16_t svc_armypanl(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
