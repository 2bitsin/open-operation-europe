#pragma once

#include "common/events.hpp"
#include "common/grid-cell.hpp"
#include "types.hpp"

uint8_t  opeu_battle_state(Game& game, uint16_t element);
uint8_t  opeu_battle_order(Game& game, uint16_t element);
void     opeu_battle_set_order(Game& game, uint16_t element, uint8_t order);
auto     opeu_battle_mark_cell(Game& game, opeu::game::GridCell cell, uint8_t side) -> void;
void     opeu_battle_clear_cell(Game& game, uint8_t x, uint8_t y);
uint16_t opeu_battle_element_flag2(Game& game, uint16_t element);
uint16_t opeu_battle_element_active(Game& game, uint16_t element);
uint16_t opeu_battle_unit_active(Game& game, uint16_t unit);
void     opeu_battle_set_mode(Game& game, uint16_t element, uint8_t mode);
uint8_t  opeu_battle_mode(Game& game, uint16_t element);
uint16_t opeu_battle_engaged(Game& game, uint16_t element);
void     opeu_battle_engage(Game& game, uint16_t element);
void     opeu_battle_disengage(Game& game, uint16_t element);
uint8_t  opeu_battle_facing(Game& game, uint16_t element);
void     opeu_battle_set_facing(Game& game, uint16_t element, uint8_t facing);
uint8_t  opeu_battle_strength_low(Game& game, uint16_t element);
uint8_t  opeu_battle_strength_high(Game& game, uint16_t element);
void     opeu_battle_set_strength(Game& game, uint16_t element, uint8_t low, uint8_t high);
uint16_t opeu_battle_cell_occupied(Game& game, uint8_t x, uint8_t y);
uint8_t  opeu_battle_group_level(Game& game, uint16_t element);
uint8_t  opeu_battle_level_gap(Game& game, uint16_t element);
uint16_t opeu_battle_level_allowed(Game& game, uint16_t element);
