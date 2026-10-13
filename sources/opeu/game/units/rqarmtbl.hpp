// The regiment table of the regiment request.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_rq_draw_units(uint8_t* ds, HostEvent event, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags);

uint16_t svc_rqarmtbl(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
