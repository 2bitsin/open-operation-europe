// Turn control of a side the player does not command through the menu.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_turn_control_other(Game& game, HostEvent event, HostEvent context_event);

// The services of this module as direct calls.
uint16_t svc_control2(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
