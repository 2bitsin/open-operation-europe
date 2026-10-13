#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_ai_plan_unit(Game& game, uint16_t element);
void opeu_battle_ai_enter_or_hold(Game& game, uint16_t element);
void opeu_battle_ai_element(Game& game, uint16_t element);
void opeu_battle_ai_assign_targets(Game& game, uint8_t side, uint8_t index);
void opeu_battle_ai_plan_side(Game& game, uint16_t unit);
void opeu_battle_select_service(Game& game);
