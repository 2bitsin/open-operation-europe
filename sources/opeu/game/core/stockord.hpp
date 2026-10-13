// The city stockpile order (command action 2): how much ammunition, fuel and food each of the side's cities keeps.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_stock_cities_fill(Game& game, BattleEvent event);
uint8_t  opeu_stock_city_enabled(Game& game, BattleEvent event);
void     opeu_stock_show(Game& game, BattleEvent event, uint16_t objective);
void     opeu_stock_show_titled(Game& game, BattleEvent event, uint16_t objective);
uint16_t opeu_stock_ask(Game& game, BattleEvent event, uint16_t what);
void     opeu_stock_edit(Game& game, BattleEvent event, uint16_t objective);
void     opeu_turn_action_2(Game& game, BattleEvent event, uint16_t segment);

// The services of the stockpile order: the routines its BattleEvent names, with their register arguments ordered.
uint16_t svc_stockord(Game& game, uint32_t service, const uint16_t* args, unsigned int count);
