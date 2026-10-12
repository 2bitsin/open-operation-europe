// The commander's evening visitor in scenario 4.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

uint16_t opeu_spy_commander_ready(uint8_t* ds, HostEvent event);
void     opeu_spy_portrait_text(uint8_t* ds, HostEvent event, uint16_t text,
                                uint16_t show);
void     opeu_spy_present_a(uint8_t* ds, HostEvent event);
void     opeu_spy_show_a(uint8_t* ds, HostEvent event);
void     opeu_spy_present_b(uint8_t* ds, HostEvent event);
void     opeu_spy_approach(uint8_t* ds, HostEvent event);
void     opeu_spy_event(uint8_t* ds, HostEvent event);

// Runs the services the module's routines call through their event.
uint16_t svc_evspy(uint8_t* ds, uint16_t id, const uint16_t* args,
                   uint16_t count);
