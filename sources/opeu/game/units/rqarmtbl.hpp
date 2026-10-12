// The regiment table of the regiment request, and the services every list table's row-draw callback calls.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_rq_draw_units(uint8_t* ds, HostEvent event, uint16_t scratch, uint8_t column, uint8_t first, uint8_t row,
                        uint8_t last, uint8_t flags);

uint16_t svc_rqarmtbl(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
// The row-draw services (TableService, common/table-rows.hpp); `module` names the caller in diagnostics.
uint16_t svc_unit_table(uint8_t* ds, const char* module, uint16_t id, const uint16_t* args, uint16_t count);
