// The city stockpile order (command action 2): how much ammunition, fuel and food each of the side's cities keeps.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_stock_cities_fill(Game& game);
uint8_t  opeu_stock_city_enabled(Game& game);
void     opeu_stock_show(Game& game, uint16_t objective);
void     opeu_stock_show_titled(Game& game, uint16_t objective);
uint16_t opeu_stock_ask(Game& game, uint16_t what);
void     opeu_stock_edit(Game& game, uint16_t objective);
void     opeu_turn_action_2(Game& game);
