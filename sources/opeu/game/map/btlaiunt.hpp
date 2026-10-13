#pragma once

#include "common/events.hpp"
#include "map/field-cell.hpp"
#include "types.hpp"

uint8_t  opeu_battle_ai_plan_low(uint8_t* ds, BattleEvent event, uint16_t element);
uint8_t  opeu_battle_ai_plan_high(uint8_t* ds, BattleEvent event, uint16_t element);
uint16_t opeu_battle_ai_unit_target(uint8_t* ds, BattleEvent event, uint16_t element);
uint16_t opeu_battle_ai_pick_target(uint8_t* ds, BattleEvent event, uint16_t element, uint16_t list, uint8_t mode);
auto     opeu_battle_ai_random_cell(uint8_t* ds, BattleEvent event, uint16_t element, uint16_t unit) -> opeu::game::FieldCell;

// The services of this module.
uint16_t svc_btlaiunt(uint8_t* ds, uint32_t service, const uint16_t* args, unsigned int count);
