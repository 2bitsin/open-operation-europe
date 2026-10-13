// The conversion team's credits, shown after the division panel when the player answers its key prompt with '!'.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_division_credits(Game& game, HostEvent event);

// Runs the services the module's routines call through their event.
uint16_t svc_credits(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
