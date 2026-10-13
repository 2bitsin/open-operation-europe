#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_mark_exits(Game& game, uint16_t element);
void     opeu_battle_unmark_exits(Game& game, uint16_t element);
uint16_t opeu_battle_lone_withdrawal(Game& game, uint16_t element);
