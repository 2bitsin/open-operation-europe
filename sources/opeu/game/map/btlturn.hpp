#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_battle_computer_turn(Game& game, BattleEvent event);
uint16_t opeu_battle_unit_turn(Game& game, BattleEvent event, uint16_t unit);
uint16_t opeu_battle_all_turns(Game& game, BattleEvent event);
uint16_t opeu_battle_side_controlled(Game& game, BattleEvent event, uint8_t side);
void     opeu_battle_toggle_numbers(Game& game, BattleEvent event);
uint16_t opeu_battle_turn_loop(Game& game, BattleEvent event);
void     opeu_battle_return_garrison(Game& game, BattleEvent event);
void     opeu_battle_daily_condition(Game& game, BattleEvent event);
void     opeu_battle_clear_positions(Game& game, BattleEvent event);

// The services of this module.
uint16_t svc_btlturn(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
