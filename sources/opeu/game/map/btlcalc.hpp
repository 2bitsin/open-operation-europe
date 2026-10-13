#pragma once

#include "common/events.hpp"
#include "types.hpp"

// opeu_battle_skill's kind: which skill of the responsible commander.
enum class BattleSkill : uint8_t {
  LEADERSHIP = 0,
  CHARM      = 1,
  BRAVERY    = 2,
  HEALTH     = 3,
  INFANTRY   = 4,
  ARTILLERY  = 5,
  ARMOR      = 6,
  EXPERIENCE = 7,
};

void     opeu_battle_pause(Game& game, uint8_t ticks);
uint8_t  opeu_battle_unit_direction(Game& game, uint16_t unit);
uint8_t  opeu_battle_skill(Game& game, uint16_t element, uint8_t kind);
uint8_t  opeu_battle_element_strength(Game& game, uint16_t element);
uint8_t  opeu_battle_odds(Game& game, uint8_t rating, uint8_t range);
uint8_t  opeu_battle_element_rating(Game& game, uint16_t attacker, uint16_t target);
uint8_t  opeu_battle_element_range(Game& game, uint16_t element);
uint8_t  opeu_battle_element_shots(Game& game, uint16_t attacker, uint16_t target);
uint8_t  opeu_battle_cell_terrain(Game& game, uint8_t x, uint8_t y);
uint16_t opeu_battle_can_enter(Game& game, uint8_t x, uint8_t y, uint16_t element);
uint8_t  opeu_battle_move_cost(Game& game, uint8_t x, uint8_t y, uint16_t element);
void     opeu_battle_step_unchecked(Game& game, uint8_t* x, uint8_t* y, uint8_t direction);
uint16_t opeu_battle_step(Game& game, uint8_t* x, uint8_t* y, uint8_t direction);
uint16_t opeu_battle_unit_controlled(Game& game, uint16_t unit);
uint16_t opeu_battle_element_at(Game& game, uint8_t x, uint8_t y);
