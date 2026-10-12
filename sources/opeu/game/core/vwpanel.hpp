// The objective and regiment information panels the map shows.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_objective_panel(uint8_t* ds, HostEvent event, uint16_t sp, uint16_t objective);
void opeu_unit_panel(uint8_t* ds, HostEvent event, uint16_t sp, uint16_t unit);

// The services of the information panels (the information views' shared services).
uint16_t svc_vwpanel(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
