// The division and mission tables' row-draw callbacks and the service table of every row-draw callback.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_tbl_draw_groups(Game& game, HostEvent event, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags);
void opeu_tbl_draw_missions(Game& game, HostEvent event, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags);

// The row-draw services (common/table-rows.hpp TableService); module names the caller in an unhandled-service report.
uint16_t svc_table(Game& game, const char* module, uint16_t id, const uint16_t* args, uint16_t count);
uint16_t svc_tblgroup(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
