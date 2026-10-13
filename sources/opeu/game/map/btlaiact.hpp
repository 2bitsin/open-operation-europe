#pragma once

#include "common/events.hpp"
#include "common/grid-cell.hpp"
#include "types.hpp"

uint16_t           opeu_battle_ai_withdraw(Game& game, uint16_t element, uint16_t unit);
uint16_t           opeu_battle_ai_try_fire(Game& game, uint16_t element, uint16_t target);
void               opeu_battle_ai_fire_or_attack(Game& game, uint16_t element, uint16_t unit);
[[nodiscard]] auto opeu_battle_ai_try_move(Game& game, uint16_t element, opeu::game::GridCell cell) -> uint16_t;
void               opeu_battle_ai_advance(Game& game, uint16_t element);
void               opeu_battle_ai_engage_best(Game& game, uint16_t element);
void               opeu_battle_ai_act(Game& game, uint16_t element);
void               opeu_battle_ai_think(Game& game, uint16_t element);
