#pragma once

#include "types.hpp"
#include "common/events.hpp"

uint16_t opeu_battle_retreat_cell_ok(uint8_t* ds, BattleEvent event, uint8_t x,
                                     uint8_t y, uint16_t unit);
void     opeu_battle_retreat_to(uint8_t* ds, BattleEvent event, uint16_t unit,
                                uint8_t x, uint8_t y);
void     opeu_battle_unit_retreat(uint8_t* ds, BattleEvent event, uint16_t unit,
                                  uint8_t side);
void     opeu_battle_morale_check(uint8_t* ds, BattleEvent event);

// The services of this module.
uint16_t svc_btlretr(uint8_t* ds, uint32_t address, const uint16_t* args,
                     unsigned int count);
