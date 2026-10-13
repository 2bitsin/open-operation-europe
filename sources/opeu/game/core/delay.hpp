// The Delay command (core/delay).
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_choose_turn_delay(uint8_t* ds, HostEvent event);

uint16_t svc_delay(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
