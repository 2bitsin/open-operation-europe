// Order handlers (see the .cpp).
#pragma once

#include "types.hpp"
#include "types/events.hpp"

uint16_t opeu_order_arrive(uint8_t* ds, HostEvent event, uint16_t sp);
uint16_t opeu_order_reach_unit(uint8_t* ds, HostEvent event, uint16_t sp);
uint8_t  opeu_order_hold_delay(uint8_t* ds, HostEvent event);
uint16_t opeu_order_hold(uint8_t* ds, HostEvent event, uint16_t sp);

uint16_t svc_ordarriv(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count);
