#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_ai_withdraw(uint8_t* ds, BattleEvent event, uint16_t element, uint16_t unit);
uint16_t opeu_battle_ai_try_fire(uint8_t* ds, BattleEvent event, uint16_t element, uint16_t target);
void     opeu_battle_ai_fire_or_attack(uint8_t* ds, BattleEvent event, uint16_t element, uint16_t unit);
uint16_t opeu_battle_ai_try_move(uint8_t* ds, BattleEvent event, uint16_t element, uint8_t x, uint8_t y);
void     opeu_battle_ai_advance(uint8_t* ds, BattleEvent event, uint16_t element);
void     opeu_battle_ai_engage_best(uint8_t* ds, BattleEvent event, uint16_t element);
void     opeu_battle_ai_act(uint8_t* ds, BattleEvent event, uint16_t element);
void     opeu_battle_ai_think(uint8_t* ds, BattleEvent event, uint16_t element);

// The services of the battle AI modules and of this module.
uint16_t svc_battle_ai(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned count);
uint16_t svc_btlaiact(uint8_t* ds, uint32_t service, const uint16_t* args, unsigned int count);
