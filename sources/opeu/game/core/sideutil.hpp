// Side helpers: the other side, the side a status points to, the side colour, the side an equipment type serves.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

uint8_t opeu_opposite_side(uint8_t side);
uint8_t opeu_side_for_status(const uint8_t* ds, uint8_t side);
void    opeu_select_side_color(uint8_t* ds, SideColorEvent event, uint8_t side);
uint8_t opeu_record_side(const uint8_t* ds, uint16_t record);

// The colour service opeu_select_side_color is called with.
void svc_sideutil(uint8_t* ds, uint16_t color);
