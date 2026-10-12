// The scenario briefings and the message hook that blinks the named city.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_brief_blink(uint8_t* ds, HostEvent event);
void opeu_brief_mark(uint8_t* ds, HostEvent event, uint8_t x, uint8_t y);
void opeu_brief_france(uint8_t* ds, HostEvent event, uint16_t scratch);
void opeu_brief_africa(uint8_t* ds, HostEvent event, uint16_t scratch);
void opeu_brief_kursk(uint8_t* ds, HostEvent event, uint16_t scratch);
void opeu_brief_normandy(uint8_t* ds, HostEvent event, uint16_t scratch);
void opeu_brief_ardennes(uint8_t* ds, HostEvent event, uint16_t scratch);
void opeu_brief_berlin(uint8_t* ds, HostEvent event, uint16_t scratch);

// The services of this module in the native build.
uint16_t svc_scenbrf(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
