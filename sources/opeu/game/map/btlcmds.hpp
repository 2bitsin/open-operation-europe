#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_order_attack(Game& game, uint16_t element, uint16_t target);
void opeu_battle_order_attack_cell(Game& game, uint16_t element);
void opeu_battle_order_move(Game& game, uint16_t element);
void opeu_battle_order_blitz(Game& game, uint16_t element, uint16_t target);
void opeu_battle_order_blitz_cell(Game& game, uint16_t element);
void opeu_battle_order_withdraw(Game& game, uint16_t element);
void opeu_battle_order_defend(Game& game, uint16_t element);
void opeu_battle_order_rest(Game& game, uint16_t element);
void opeu_battle_order_refused(Game& game, uint8_t kind);
