#pragma once

#include "types.hpp"
#include "types/events.hpp"

uint16_t opeu_battle_random_target(uint8_t* ds, BattleEvent event,
                                   uint16_t element);
void opeu_battle_element_turn(uint8_t* ds, BattleEvent event, uint16_t element);
uint16_t opeu_battle_unit_fight(uint8_t* ds, BattleEvent event, uint16_t unit);
uint16_t opeu_battle_round(uint8_t* ds, BattleEvent event);
uint16_t opeu_battle_rounds(uint8_t* ds, BattleEvent event);
void     opeu_battle_finish_turn(uint8_t* ds, BattleEvent event);
void     opeu_battle_finish_none(uint8_t* ds, BattleEvent event);

// The services of this module.
uint16_t svc_btlfight(uint8_t* ds, uint32_t address, const uint16_t* args,
                      unsigned int count);
