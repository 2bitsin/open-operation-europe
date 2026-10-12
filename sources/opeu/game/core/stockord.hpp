// The city stockpile order (command action 2): how much ammunition, fuel and food each of the side's cities keeps.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_stock_cities_fill(uint8_t* ds, BattleEvent event, uint16_t sp);
uint8_t  opeu_stock_city_enabled(uint8_t* ds, BattleEvent event, uint16_t sp);
void     opeu_stock_show(uint8_t* ds, BattleEvent event, uint16_t sp, uint16_t objective);
void     opeu_stock_show_titled(uint8_t* ds, BattleEvent event, uint16_t sp, uint16_t objective);
uint16_t opeu_stock_ask(uint8_t* ds, BattleEvent event, uint16_t sp, uint16_t what);
void     opeu_stock_edit(uint8_t* ds, BattleEvent event, uint16_t sp, uint16_t objective);
void     opeu_turn_action_2(uint8_t* ds, BattleEvent event, uint16_t sp, uint16_t segment);

// The services of the stockpile order: the routines its BattleEvent names, with their register arguments ordered.
uint16_t svc_stockord(uint8_t* ds, uint32_t service, const uint16_t* args, unsigned int count);
