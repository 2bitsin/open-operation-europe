// The scenario briefings and the message hook that blinks the named city.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_brief_blink(Game& game, HostEvent event);
void opeu_brief_mark(Game& game, HostEvent event, uint8_t x, uint8_t y);
void opeu_brief_france(Game& game, HostEvent event);
void opeu_brief_africa(Game& game, HostEvent event);
void opeu_brief_kursk(Game& game, HostEvent event);
void opeu_brief_normandy(Game& game, HostEvent event);
void opeu_brief_ardennes(Game& game, HostEvent event);
void opeu_brief_berlin(Game& game, HostEvent event);

// The services of this module in the native build.
uint16_t svc_scenbrf(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
