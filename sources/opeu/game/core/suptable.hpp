// The supply-allocation tables: the types the side holds and the slots allocated to units, and their list views.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

uint8_t opeu_supply_unit_eligible(uint8_t* ds);
void    opeu_supply_reset(uint8_t* ds);
uint8_t opeu_supply_true(void);
void    opeu_supply_draw_types(uint8_t* ds, HostNotify event, uint16_t scratch,
                               uint8_t column, uint8_t first, uint8_t row,
                               uint8_t last, uint8_t flags);
uint8_t opeu_supply_return_slot(uint8_t* ds, HostNotify event);
void    opeu_supply_draw_slots(uint8_t* ds, HostNotify event, uint16_t scratch,
                               uint8_t column, uint8_t first, uint8_t row,
                               uint8_t last, uint8_t flags);

// The list layout, text and highlight services of the table views as direct calls.
void svc_suptable(uint8_t* ds, uint16_t id, const uint16_t* args,
                  uint16_t count);
