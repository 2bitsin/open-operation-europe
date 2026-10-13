// Order handlers (see the .cpp).
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_order_destroy_loss(Game& game, HostEvent event);
void     opeu_order_destroy_present(Game& game, HostEvent event);
uint16_t opeu_order_destroy_run(Game& game, HostEvent event);

uint16_t svc_ordbridg(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
