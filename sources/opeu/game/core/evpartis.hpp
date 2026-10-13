// The hurricane and partisan events.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_hurricane_present(Game& game, HostEvent event);
void opeu_partisan_event(Game& game, HostEvent event);

// Runs the services the module's routines call through their event.
uint16_t svc_evpartis(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
