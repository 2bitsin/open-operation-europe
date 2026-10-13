// The commander panel of the daily display and the scenario's turn context.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_daily_context(Game& game, HostEvent event, uint8_t side);
void opeu_turn_pre_context(Game& game, HostEvent event);

// Runs the services the module's routines call through their event.
uint16_t svc_context(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
