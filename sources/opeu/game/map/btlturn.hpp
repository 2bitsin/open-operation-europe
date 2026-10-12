#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_battle_computer_turn(uint8_t* ds, BattleEvent event);
uint16_t opeu_battle_unit_turn(uint8_t* ds, BattleEvent event, uint16_t unit);
uint16_t opeu_battle_all_turns(uint8_t* ds, BattleEvent event);
uint16_t opeu_battle_side_controlled(uint8_t* ds, BattleEvent event, uint8_t side);
void     opeu_battle_toggle_numbers(uint8_t* ds, BattleEvent event, uint16_t frame);
uint16_t opeu_battle_turn_loop(uint8_t* ds, BattleEvent event, uint16_t frame);
void     opeu_battle_return_garrison(uint8_t* ds, BattleEvent event);
void     opeu_battle_daily_condition(uint8_t* ds, BattleEvent event);
void     opeu_battle_clear_positions(uint8_t* ds, BattleEvent event);

// The services of this module.
uint16_t svc_btlturn(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count);
