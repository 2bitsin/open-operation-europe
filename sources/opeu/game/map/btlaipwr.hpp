#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_ai_side_power(Game& game, BattleEvent event, uint8_t side, uint16_t all);
uint8_t  opeu_battle_ai_power_ratio(Game& game, BattleEvent event, uint16_t unit);
uint8_t  opeu_battle_ai_choose_plan(Game& game, BattleEvent event, uint16_t unit);

// The services of this module.
uint16_t svc_btlaipwr(Game& game, uint32_t service, const uint16_t* args, unsigned int count);
