// Row-draw callbacks of a list table (see the .cpp).
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_tbl_draw_types(uint8_t* ds, HostEvent event, uint16_t scratch, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags);

uint16_t svc_tbltype(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
