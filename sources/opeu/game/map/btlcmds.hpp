#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_order_attack(Game& game, BattleEvent event, uint16_t element, uint16_t target);
void opeu_battle_order_attack_cell(Game& game, BattleEvent event, uint16_t element);
void opeu_battle_order_move(Game& game, BattleEvent event, uint16_t element);
void opeu_battle_order_blitz(Game& game, BattleEvent event, uint16_t element, uint16_t target);
void opeu_battle_order_blitz_cell(Game& game, BattleEvent event, uint16_t element);
void opeu_battle_order_withdraw(Game& game, BattleEvent event, uint16_t element);
void opeu_battle_order_defend(Game& game, BattleEvent event, uint16_t element);
void opeu_battle_order_rest(Game& game, BattleEvent event, uint16_t element);
void opeu_battle_order_refused(Game& game, BattleEvent event, uint8_t kind);

// The services of this module.
uint16_t svc_battle_orders(Game& game, uint32_t address, const uint16_t* args, unsigned count);
uint16_t svc_btlcmds(Game& game, uint32_t service, const uint16_t* args, unsigned int count);
