#pragma once

#include "types.hpp"
#include "common/events.hpp"

void     opeu_battle_menu_place(uint8_t* ds, BattleEvent event, uint16_t frame);
uint16_t opeu_battle_orders_menu(uint8_t* ds, BattleEvent event,
                                 uint16_t element, uint16_t frame);

// The services of this module.
uint16_t svc_btlmenu(uint8_t* ds, uint32_t service, const uint16_t* args,
                     unsigned int count);
