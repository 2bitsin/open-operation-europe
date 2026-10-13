#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_battle_state(Game& game, BattleEvent event, uint16_t element);
uint8_t  opeu_battle_order(Game& game, BattleEvent event, uint16_t element);
void     opeu_battle_set_order(Game& game, BattleEvent event, uint16_t element, uint8_t order);
void     opeu_battle_mark_cell(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint8_t side);
void     opeu_battle_clear_cell(Game& game, BattleEvent event, uint8_t x, uint8_t y);
uint16_t opeu_battle_element_flag2(Game& game, BattleEvent event, uint16_t element);
uint16_t opeu_battle_element_active(Game& game, BattleEvent event, uint16_t element);
uint16_t opeu_battle_unit_active(Game& game, BattleEvent event, uint16_t unit);
void     opeu_battle_set_mode(Game& game, BattleEvent event, uint16_t element, uint8_t mode);
uint8_t  opeu_battle_mode(Game& game, BattleEvent event, uint16_t element);
uint16_t opeu_battle_engaged(Game& game, BattleEvent event, uint16_t element);
void     opeu_battle_engage(Game& game, BattleEvent event, uint16_t element);
void     opeu_battle_disengage(Game& game, BattleEvent event, uint16_t element);
uint8_t  opeu_battle_facing(Game& game, BattleEvent event, uint16_t element);
void     opeu_battle_set_facing(Game& game, BattleEvent event, uint16_t element, uint8_t facing);
uint8_t  opeu_battle_strength_low(Game& game, BattleEvent event, uint16_t element);
uint8_t  opeu_battle_strength_high(Game& game, BattleEvent event, uint16_t element);
void     opeu_battle_set_strength(Game& game, BattleEvent event, uint16_t element, uint8_t low, uint8_t high);
uint16_t opeu_battle_cell_occupied(Game& game, BattleEvent event, uint8_t x, uint8_t y);
uint16_t opeu_battle_cell_is(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint8_t side);
uint8_t  opeu_battle_group_level(Game& game, BattleEvent event, uint16_t element);
uint8_t  opeu_battle_level_gap(Game& game, BattleEvent event, uint16_t element);
uint16_t opeu_battle_level_allowed(Game& game, BattleEvent event, uint16_t element);

// The services of this module.
uint16_t svc_btlunit(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
