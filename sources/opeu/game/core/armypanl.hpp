// The division panel's regiments, their slot grids and element rows.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_panel_slots(Game& game, int y, uint8_t count);
uint32_t opeu_element_icon(Game const& game, uint16_t element);
void     opeu_panel_elements(Game& game, uint16_t unit, int y);
void     opeu_panel_battle_frame(Game& game, int y);
void     opeu_division_regiments(Game& game, uint16_t units, uint16_t person);
