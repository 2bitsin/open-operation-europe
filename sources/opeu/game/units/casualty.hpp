// Casualties for every element of a regiment.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_unit_casualties(uint8_t* ds, HostEvent event, uint16_t frame, uint16_t unit, uint16_t percent, uint16_t kind,
                          uint16_t c, uint16_t d, uint16_t e);

uint16_t svc_casualty(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
