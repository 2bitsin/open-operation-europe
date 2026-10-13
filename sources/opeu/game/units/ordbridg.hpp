// Order handlers (see the .cpp).
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_order_destroy_loss(Game& game);
void     opeu_order_destroy_present(Game& game);
uint16_t opeu_order_destroy_run(Game& game);
