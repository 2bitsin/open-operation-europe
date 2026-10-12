// The marker on the setup screen.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_setup_marker_show(uint8_t* ds, HostEvent event, uint8_t index);
void opeu_setup_marker_hide(uint8_t* ds, HostEvent event, uint8_t index);

// The services the module calls through its event.
uint16_t svc_setmark(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
