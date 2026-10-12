#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_map_highlight_view(uint8_t* ds, HostEvent event, uint16_t frame,
                             uint8_t x, uint8_t y);
void opeu_map_highlight_overview(uint8_t* ds, HostEvent event, uint16_t frame,
                                 uint8_t x, uint8_t y);

// The services of this module.
uint16_t svc_highlite(uint8_t* ds, uint16_t kind, const uint16_t* args,
                      uint16_t count);
