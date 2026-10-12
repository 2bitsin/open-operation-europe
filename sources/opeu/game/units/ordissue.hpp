// Orders command: the order menu of a regiment or a whole division.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_unit_order_menu(uint8_t* ds, HostEvent event, uint16_t sp,
                          uint8_t group);

uint16_t svc_ordissue(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count);
