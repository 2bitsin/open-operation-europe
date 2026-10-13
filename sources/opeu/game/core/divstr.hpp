// The division strength line of the current side's army panel: infantry, tanks and artillery of a division.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_show_division_strength(Game& game, HostNotify event, uint8_t div);

// Runs the services the module's routines call through their event.
void svc_divstr(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
