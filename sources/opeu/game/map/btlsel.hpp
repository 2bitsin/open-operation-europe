#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_mouse_select(Game& game, BattleEvent event, uint16_t show_info);
uint16_t opeu_battle_key_select(Game& game, BattleEvent event, uint16_t show_info);
uint16_t opeu_battle_select_cell(Game& game, BattleEvent event);

// The services of this module.
uint16_t svc_btlsel(Game& game, uint32_t service, const uint16_t* args, unsigned int count);
