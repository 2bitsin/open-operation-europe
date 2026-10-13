// The division panel's regiments, their slot grids and element rows.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_panel_slots(uint8_t* ds, HostEvent event, int y, uint8_t count);
uint32_t opeu_element_icon(const uint8_t* ds, uint16_t element);
void     opeu_panel_elements(uint8_t* ds, HostEvent event, uint16_t unit, int y);
void     opeu_panel_battle_frame(uint8_t* ds, HostEvent event, int y);
void     opeu_division_regiments(uint8_t* ds, HostEvent event, uint16_t units, uint16_t person);

// Runs the services the module's routines call through their event.
uint16_t svc_armypanl(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
