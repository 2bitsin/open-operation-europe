// Order handlers (see the .cpp).
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_order_arrive(Game& game);
uint16_t opeu_order_reach_unit(Game& game);
uint8_t  opeu_order_hold_delay(Game& game);
uint16_t opeu_order_hold(Game& game);
