// The supply-allocation tables: the types the side holds and the slots allocated to units, and their list views.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_supply_unit_eligible(Game& game);
void    opeu_supply_reset(Game& game);
uint8_t opeu_supply_true(void);
void    opeu_supply_draw_types(Game& game, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags);
uint8_t opeu_supply_return_slot(Game& game);
void    opeu_supply_draw_slots(Game& game, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags);
