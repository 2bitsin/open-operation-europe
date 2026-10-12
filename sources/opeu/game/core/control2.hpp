// Turn control of a side the player does not command through the menu.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_turn_control_other(uint8_t* ds, HostEvent event,
                             HostEvent context_event);

// The services of this module as direct calls.
uint16_t svc_control2(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count);
