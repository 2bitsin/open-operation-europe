#pragma once

#include "types.hpp"
#include "common/events.hpp"

void     opeu_battle_ai_hold(uint8_t* ds, BattleEvent event, uint16_t element);
uint16_t opeu_battle_ai_entry_ok(uint8_t* ds, BattleEvent event, uint8_t x,
                                 uint8_t y, uint8_t direction);
uint16_t opeu_battle_ai_find_entry(uint8_t* ds, BattleEvent event, uint16_t x,
                                   uint16_t y, uint16_t element);
uint16_t opeu_battle_ai_enter(uint8_t* ds, BattleEvent event, uint16_t element);

// The services of this module.
uint16_t svc_btlaient(uint8_t* ds, uint32_t service, const uint16_t* args,
                      unsigned int count);
