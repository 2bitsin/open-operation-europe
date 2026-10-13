#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_map_highlight_view(Game& game, HostEvent event, uint8_t x, uint8_t y);
void opeu_map_highlight_overview(Game& game, HostEvent event, uint8_t x, uint8_t y);

// The services of this module.
uint16_t svc_highlite(Game& game, uint16_t kind, const uint16_t* args, uint16_t count);
