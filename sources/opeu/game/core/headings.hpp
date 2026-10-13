// The army heading (side name, commander, HQ) and the division heading (number, commander, portrait) of the panels.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_army_heading(Game& game, HostNotify event, SideColorEvent color_event, uint16_t person);
void opeu_division_heading(Game& game, HostNotify event, uint16_t person);

// Runs the services the module's routines call through their event.
void svc_headings(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
