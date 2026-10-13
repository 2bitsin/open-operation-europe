#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_mark_exits(Game& game, BattleEvent event, uint16_t element);
void     opeu_battle_unmark_exits(Game& game, BattleEvent event, uint16_t element);
uint16_t opeu_battle_lone_withdrawal(Game& game, BattleEvent event, uint16_t element);

// The services of this module.
uint16_t svc_btlreach(Game& game, uint32_t service, const uint16_t* args, unsigned int count);
