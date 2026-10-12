// The general-list callbacks of the staff and division assignment screen.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

uint8_t opeu_general_assign_choice(uint8_t* ds, HostEvent event,
                                   uint8_t choice);
void opeu_general_list_draw_rows(uint8_t* ds, HostEvent event, uint8_t column,
                                 uint8_t first, uint8_t row, uint8_t last,
                                 uint8_t flags);

// The services the module calls out to.
uint16_t svc_genpick(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count);
