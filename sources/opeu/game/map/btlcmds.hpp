#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_battle_order_attack(uint8_t* ds, BattleEvent event, uint16_t element,
                              uint16_t target);
void opeu_battle_order_attack_cell(uint8_t* ds, BattleEvent event,
                                   uint16_t element);
void opeu_battle_order_move(uint8_t* ds, BattleEvent event, uint16_t element);
void opeu_battle_order_blitz(uint8_t* ds, BattleEvent event, uint16_t element,
                             uint16_t target);
void opeu_battle_order_blitz_cell(uint8_t* ds, BattleEvent event,
                                  uint16_t element);
void opeu_battle_order_withdraw(uint8_t* ds, BattleEvent event,
                                uint16_t element);
void opeu_battle_order_defend(uint8_t* ds, BattleEvent event, uint16_t element);
void opeu_battle_order_rest(uint8_t* ds, BattleEvent event, uint16_t element);
void opeu_battle_order_refused(uint8_t* ds, BattleEvent event, uint8_t kind);

// The services of this module.
uint16_t svc_battle_orders(uint8_t* ds, uint32_t address, const uint16_t* args,
                           unsigned count);
uint16_t svc_btlcmds(uint8_t* ds, uint32_t service, const uint16_t* args,
                     unsigned int count);
