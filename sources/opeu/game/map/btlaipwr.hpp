#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_ai_side_power(Game& game, uint8_t side, uint16_t all);
uint8_t  opeu_battle_ai_power_ratio(Game& game, uint16_t unit);
uint8_t  opeu_battle_ai_choose_plan(Game& game, uint16_t unit);
