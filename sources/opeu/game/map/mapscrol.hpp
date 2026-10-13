#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_view_cell_clear(Game& game, HostEvent event, int16_t x, int16_t y);
void opeu_view_redraw(Game& game, HostEvent event);
void opeu_view_scroll(Game& game, HostEvent event, int16_t x, int16_t y);
void opeu_view_scroll_to(Game& game, HostEvent event, int16_t x, int16_t y);
void opeu_map_scroll_to(Game& game, HostEvent event, int16_t x, int16_t y, uint8_t side);

// The services of this module.
uint16_t svc_mapscrol(Game& game, uint16_t kind, const uint16_t* args, uint16_t count);
