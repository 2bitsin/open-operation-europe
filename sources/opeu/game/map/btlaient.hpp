#pragma once

#include "common/events.hpp"
#include "map/grid-cell.hpp"
#include "types.hpp"

void               opeu_battle_ai_hold(Game& game, uint16_t element);
[[nodiscard]] auto opeu_battle_ai_entry_ok(Game& game, opeu::game::GridCell cell, uint8_t direction) -> uint16_t;
uint16_t           opeu_battle_ai_enter(Game& game, uint16_t element);
