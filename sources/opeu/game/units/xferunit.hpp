// The supply orders and the loading and storing of the supply-transfer table.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_transfer_load_unit(Game& game, HostWordsEvent event, uint16_t unit, uint8_t side);
void     opeu_transfer_load_objective(Game& game, HostWordsEvent event, uint16_t objective);
void     opeu_transfer_units(Game& game, HostWordsEvent event, uint16_t first, uint16_t second);
void     opeu_transfer_objective(Game& game, HostWordsEvent event, uint16_t unit, uint16_t objective);
uint8_t  opeu_transfer_level(Game& game, HostWordsEvent event);
uint16_t opeu_order_supply_objective(Game& game, HostWordsEvent event);
uint16_t opeu_order_supply_unit(Game& game, HostWordsEvent event);

uint16_t svc_xferunit(Game& game, uint16_t id, uint16_t* args, uint16_t count);
