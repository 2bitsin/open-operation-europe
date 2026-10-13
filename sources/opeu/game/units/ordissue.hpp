// Orders command: the order menu of a regiment or a whole division.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_unit_order_menu(uint8_t* ds, HostEvent event, uint8_t group);

uint16_t svc_ordissue(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
