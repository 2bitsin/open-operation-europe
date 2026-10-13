#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_cursor_info(Game& game, BattleEvent event);

// The services of this module.
uint16_t svc_btlinfo(Game& game, uint32_t service, const uint16_t* args, unsigned int count);
