#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_mouse_select(uint8_t* ds, BattleEvent event, uint16_t show_info, uint16_t frame);
uint16_t opeu_battle_key_select(uint8_t* ds, BattleEvent event, uint16_t show_info);
uint16_t opeu_battle_select_cell(uint8_t* ds, BattleEvent event, uint16_t sp);

// The services of this module.
uint16_t svc_btlsel(uint8_t* ds, uint32_t service, const uint16_t* args, unsigned int count);
