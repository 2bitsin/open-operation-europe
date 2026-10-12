// Order handlers (see the .cpp).
#pragma once

#include "types.hpp"
#include "types/events.hpp"

uint8_t  opeu_order_destroy_loss(uint8_t* ds, HostEvent event);
void     opeu_order_destroy_present(uint8_t* ds, HostEvent event);
uint16_t opeu_order_destroy_run(uint8_t* ds, HostEvent event, uint16_t frame);

uint16_t svc_ordbridg(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count);
