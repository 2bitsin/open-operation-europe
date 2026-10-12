// Orders command (turn action 1).
#pragma once

#include "types.hpp"
#include "common/events.hpp"

uint8_t opeu_order_units_fill(uint8_t* ds, uint8_t group);
void    opeu_turn_action_1(uint8_t* ds, HostEvent event, uint16_t sp,
                           uint16_t segment);

uint16_t svc_ordlist(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count);
