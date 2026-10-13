#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_battle_ai_hold(Game& game, uint16_t element);
uint16_t opeu_battle_ai_entry_ok(Game& game, uint8_t x, uint8_t y, uint8_t direction);
uint16_t opeu_battle_ai_find_entry(Game& game, uint16_t x, uint16_t y, uint16_t element);
uint16_t opeu_battle_ai_enter(Game& game, uint16_t element);
