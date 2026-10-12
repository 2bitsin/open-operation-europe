#pragma once

#include "types.hpp"
#include "common/events.hpp"

uint8_t  opeu_battle_path_cost(uint8_t* ds, BattleEvent event, uint8_t x,
                               uint8_t y, uint16_t element);
uint8_t  opeu_battle_path_cost_any(uint8_t* ds, BattleEvent event, uint8_t x,
                                   uint8_t y, uint16_t element);
uint16_t opeu_battle_path_step(uint8_t* ds, BattleEvent event, uint16_t x,
                               uint16_t y, uint16_t frame);
uint16_t opeu_battle_path_flood(uint8_t* ds, BattleEvent event, uint8_t x,
                                uint8_t y, uint8_t to_x, uint8_t to_y,
                                uint16_t element, uint8_t mode, uint16_t frame);
void     opeu_battle_report_sighted(uint8_t* ds, BattleEvent event,
                                    uint16_t element, uint16_t frame);
uint16_t opeu_battle_spot_chance(uint8_t* ds, BattleEvent event,
                                 uint16_t element, uint16_t other);
void opeu_battle_spot_enemies(uint8_t* ds, BattleEvent event, uint16_t element,
                              uint16_t frame);

// The services of this module.
uint16_t svc_btlpath(uint8_t* ds, uint32_t address, const uint16_t* args,
                     unsigned int count);
