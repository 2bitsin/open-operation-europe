#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_battle_computer_turn(Game& game);
uint16_t opeu_battle_unit_turn(Game& game, uint16_t unit);
uint16_t opeu_battle_all_turns(Game& game);
uint16_t opeu_battle_side_controlled(Game& game, uint8_t side);
void     opeu_battle_toggle_numbers(Game& game);
uint16_t opeu_battle_turn_loop(Game& game);
void     opeu_battle_return_garrison(Game& game);
void     opeu_battle_daily_condition(Game& game);
void     opeu_battle_clear_positions(Game& game);
