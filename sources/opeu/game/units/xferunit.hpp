// The supply orders and the loading and storing of the supply-transfer table.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_transfer_load_unit(uint8_t* ds, HostWordsEvent event, uint16_t unit,
                             uint8_t side);
void opeu_transfer_load_objective(uint8_t* ds, HostWordsEvent event,
                                  uint16_t objective);
void opeu_transfer_units(uint8_t* ds, HostWordsEvent event, uint16_t first,
                         uint16_t second);
void opeu_transfer_objective(uint8_t* ds, HostWordsEvent event, uint16_t unit,
                             uint16_t objective);
uint8_t  opeu_transfer_level(uint8_t* ds, HostWordsEvent event);
uint16_t opeu_order_supply_objective(uint8_t* ds, HostWordsEvent event);
uint16_t opeu_order_supply_unit(uint8_t* ds, HostWordsEvent event);

uint16_t svc_xferunit(uint8_t* ds, uint16_t id, uint16_t* args, uint16_t count);
