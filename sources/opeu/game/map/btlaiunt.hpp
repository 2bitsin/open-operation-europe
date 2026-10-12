#pragma once

#include "types.hpp"
#include "common/events.hpp"

uint8_t  opeu_battle_ai_plan_low(uint8_t* ds, BattleEvent event,
                                 uint16_t element);
uint8_t  opeu_battle_ai_plan_high(uint8_t* ds, BattleEvent event,
                                  uint16_t element);
uint16_t opeu_battle_ai_unit_target(uint8_t* ds, BattleEvent event,
                                    uint16_t element);
uint16_t opeu_battle_ai_pick_target(uint8_t* ds, BattleEvent event,
                                    uint16_t element, uint16_t list,
                                    uint8_t mode);
void opeu_battle_ai_random_cell(uint8_t* ds, BattleEvent event, uint16_t x,
                                uint16_t y, uint16_t element, uint16_t unit);

// The services of this module.
uint16_t svc_btlaiunt(uint8_t* ds, uint32_t service, const uint16_t* args,
                      unsigned int count);
