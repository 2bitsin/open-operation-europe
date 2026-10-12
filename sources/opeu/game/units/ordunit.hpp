// Regiment order helpers of the Orders command.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

void    opeu_cancel_unit_orders(uint8_t* ds, HostEvent event, uint16_t unit,
                                uint16_t person);
uint8_t opeu_supply_unit_enabled(uint8_t* ds);
void    opeu_order_layout(uint8_t* ds, uint8_t kind);
void    opeu_order_layout_pair(uint8_t* ds, uint16_t kind);

uint16_t svc_ordunit(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count);
