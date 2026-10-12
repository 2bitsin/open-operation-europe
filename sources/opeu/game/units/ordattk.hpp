// Order handlers (see the .cpp).
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_order_attack_allowed(uint8_t* ds, HostEvent event, uint16_t frame, uint8_t x, uint8_t y);
void     opeu_order_attack_present(uint8_t* ds, HostEvent event);
uint16_t opeu_order_attack_start(uint8_t* ds, HostEvent event, uint16_t frame, uint16_t name, uint8_t x, uint8_t y);
uint8_t  opeu_order_occupy_loss(uint8_t* ds, HostEvent event);
void     opeu_order_occupied_present(uint8_t* ds, HostEvent event);
uint16_t opeu_order_occupy_run(uint8_t* ds, HostEvent event, uint16_t frame);
uint8_t  opeu_order_attack_loss(uint8_t* ds, HostEvent event);
uint16_t opeu_order_attack_run(uint8_t* ds, HostEvent event, uint16_t frame);

uint16_t svc_ordattk(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
