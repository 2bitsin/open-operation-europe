#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_battle_supply_notice(uint8_t* ds, BattleEvent event, uint16_t element,
                               uint8_t supply, uint16_t frame);
uint16_t opeu_battle_target_in_range(uint8_t* ds, BattleEvent event,
                                     uint16_t element);
uint16_t opeu_battle_cell_in_range(uint8_t* ds, BattleEvent event,
                                   uint16_t element);
void opeu_battle_animate_facing(uint8_t* ds, BattleEvent event,
                                uint16_t element, uint8_t mode, uint16_t frame);
uint8_t  opeu_battle_step_into(uint8_t* ds, BattleEvent event, uint16_t element,
                               uint8_t x, uint8_t y, uint16_t frame);
uint8_t  opeu_battle_next_step(uint8_t* ds, BattleEvent event, uint16_t element,
                               uint16_t x, uint16_t y, uint16_t frame);
uint16_t opeu_battle_element_path_step(uint8_t* ds, BattleEvent event,
                                       uint16_t element, uint16_t x, uint16_t y,
                                       uint16_t frame);

// The services of this module.
uint16_t svc_btlengag(uint8_t* ds, uint32_t address, const uint16_t* args,
                      unsigned int count);
