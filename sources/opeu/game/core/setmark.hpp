// The marker on the setup screen.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_setup_marker_show(Game& game, HostEvent event, uint8_t index);
void opeu_setup_marker_hide(Game& game, HostEvent event, uint8_t index);

// The services the module calls through its event.
uint16_t svc_setmark(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
