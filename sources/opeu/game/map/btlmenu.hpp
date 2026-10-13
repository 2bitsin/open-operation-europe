#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_battle_menu_place(Game& game, BattleEvent event);
uint16_t opeu_battle_orders_menu(Game& game, BattleEvent event, uint16_t element);

// The services of this module.
uint16_t svc_btlmenu(Game& game, uint32_t service, const uint16_t* args, unsigned int count);
