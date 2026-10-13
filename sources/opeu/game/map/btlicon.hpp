#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_battle_element_icon(uint8_t* ds, BattleEvent event, uint16_t element);
uint16_t opeu_battle_element_visible(uint8_t* ds, BattleEvent event, uint16_t element);
void     opeu_battle_draw_count(uint8_t* ds, BattleEvent event, uint16_t count, uint8_t mode, uint16_t side);
uint16_t opeu_battle_code_low(uint8_t* ds, BattleEvent event, uint8_t code);
void     opeu_battle_draw_icon(uint8_t* ds, BattleEvent event, uint16_t element, uint8_t side, uint8_t mode);

// The services of this module.
uint16_t svc_btlicon(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count);
