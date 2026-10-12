// The division strength line of the current side's army panel: infantry, tanks and artillery of a division.
#pragma once

#include "types.hpp"
#include "types/events.hpp"

void opeu_show_division_strength(uint8_t* ds, HostNotify event, uint8_t div);

// Runs the services the module's routines call through their event.
void svc_divstr(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
