// The end of a battle: occupation, morale, leaders' experience, resumed orders and the loss report.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_end_can_occupy(uint8_t* ds, uint8_t x, uint8_t y, uint16_t unit);
void     opeu_end_occupy_present(uint8_t* ds, HostEvent event);
void     opeu_end_occupy(uint8_t* ds, HostEvent event);
void     opeu_end_morale(uint8_t* ds);
void     opeu_end_leaders(uint8_t* ds);
void     opeu_end_select(uint8_t* ds, uint8_t side);
void     opeu_end_orders(uint8_t* ds);
void     opeu_end_nothing(void);
void     opeu_end_losses(uint8_t* ds, HostEvent event);
void     opeu_notice_end(uint8_t* ds, HostEvent event);

// The services the module calls through its event.
uint16_t svc_notend(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
