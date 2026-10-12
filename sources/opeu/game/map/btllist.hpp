#pragma once

#include "types.hpp"
#include "types/events.hpp"

uint8_t  opeu_battle_element_side(uint8_t* ds, BattleEvent event,
                                  uint16_t element);
uint8_t  opeu_battle_unit_side(uint8_t* ds, BattleEvent event, uint16_t unit);
uint16_t opeu_battle_element_unit(uint8_t* ds, BattleEvent event,
                                  uint16_t element);
uint8_t  opeu_battle_element_count(uint8_t* ds, BattleEvent event,
                                   uint16_t unit);

// The services of this module.
uint16_t svc_btllist(uint8_t* ds, uint32_t address, const uint16_t* args,
                     unsigned int count);
