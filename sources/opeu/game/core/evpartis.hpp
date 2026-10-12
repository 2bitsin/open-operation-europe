// The hurricane and partisan events.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_hurricane_present(uint8_t* ds, HostEvent event);
void opeu_partisan_event(uint8_t* ds, HostEvent event);

// Runs the services the module's routines call through their event.
uint16_t svc_evpartis(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
