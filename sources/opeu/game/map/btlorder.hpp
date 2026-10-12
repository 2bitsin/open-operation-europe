#pragma once

#include "types.hpp"
#include "types/events.hpp"

uint8_t opeu_battle_execute_order(uint8_t* ds, BattleEvent event,
                                  uint16_t element, uint8_t order,
                                  uint16_t frame);
void    opeu_battle_attack_or_hold(uint8_t* ds, BattleEvent event,
                                   uint16_t element, uint16_t target);
void    opeu_battle_continue_order(uint8_t* ds, BattleEvent event,
                                   uint16_t element, uint16_t frame);
void    opeu_battle_advance(uint8_t* ds, BattleEvent event, uint16_t element,
                            uint16_t frame);
void opeu_battle_seek_target(uint8_t* ds, BattleEvent event, uint16_t element,
                             uint16_t frame);

// The services of this module.
uint16_t svc_btlorder(uint8_t* ds, uint32_t address, const uint16_t* args,
                      unsigned int count);
