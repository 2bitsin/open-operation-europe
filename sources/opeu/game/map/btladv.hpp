#pragma once

#include "common/events.hpp"
#include "map/grid-cell.hpp"
#include "types.hpp"

auto opeu_battle_exit_to_map(Game& game, uint16_t element, opeu::game::GridCell cell) -> void;
void opeu_battle_element_withdraw(Game& game, uint16_t element);
void opeu_battle_withdraw_step(Game& game, uint16_t element);
void opeu_battle_hold_step(Game& game, uint16_t element);
void opeu_battle_element_step(Game& game, uint16_t element);
