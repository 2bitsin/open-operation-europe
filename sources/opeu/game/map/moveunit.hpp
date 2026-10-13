#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_move_unit(uint8_t* ds, HostEvent event, uint16_t repeat);

// The services of this module.
uint16_t svc_moveunit(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
