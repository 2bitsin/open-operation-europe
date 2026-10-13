// The objective and regiment information panels the map shows.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_objective_panel(Game& game, HostEvent event, uint16_t objective);
void opeu_unit_panel(Game& game, HostEvent event, uint16_t unit);

// The services of the information panels (the information views' shared services).
uint16_t svc_vwpanel(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
