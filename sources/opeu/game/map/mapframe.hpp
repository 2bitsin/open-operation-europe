#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_view_number(uint8_t* ds, HostEvent event, uint16_t x, uint16_t y,
                      uint16_t value);
void opeu_scale_label_x(uint8_t* ds, HostEvent event, uint16_t x, uint16_t y,
                        uint16_t back, uint16_t fore, int16_t value);
void opeu_scale_label_y(uint8_t* ds, HostEvent event, uint16_t x, uint16_t y,
                        uint16_t back, uint16_t fore, int16_t value);
void opeu_overview_frame(uint8_t* ds, HostEvent event);
void opeu_view_frame(uint8_t* ds, HostEvent event, int16_t x, int16_t y);

// The services of this module.
uint16_t svc_mapframe(uint8_t* ds, uint16_t kind, const uint16_t* args,
                      uint16_t count);
