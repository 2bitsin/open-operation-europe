// Order handlers (see the .cpp).
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_order_arrive(Game& game, HostEvent event);
uint16_t opeu_order_reach_unit(Game& game, HostEvent event);
uint8_t  opeu_order_hold_delay(Game& game, HostEvent event);
uint16_t opeu_order_hold(Game& game, HostEvent event);

uint16_t svc_ordarriv(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
