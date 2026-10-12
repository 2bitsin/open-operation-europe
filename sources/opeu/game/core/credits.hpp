// The conversion team's credits, shown after the division panel when the player answers its key prompt with '!'.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_division_credits(uint8_t* ds, HostEvent event);

// Runs the services the module's routines call through their event.
uint16_t svc_credits(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
