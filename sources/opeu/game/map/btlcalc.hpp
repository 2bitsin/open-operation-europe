#pragma once

#include "types.hpp"
#include "common/events.hpp"

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

void     opeu_battle_pause(uint8_t* ds, BattleService service, uint8_t ticks);
uint8_t  opeu_battle_unit_direction(uint8_t* ds, BattleService service,
                                    uint16_t unit);
uint8_t  opeu_battle_skill(uint8_t* ds, BattleService service, uint16_t element,
                           uint8_t kind);
uint8_t  opeu_battle_element_strength(uint8_t* ds, BattleService service,
                                      uint16_t element);
uint8_t  opeu_battle_odds(uint8_t* ds, BattleService service, uint8_t rating,
                          uint8_t range);
uint8_t  opeu_battle_element_rating(uint8_t* ds, BattleService service,
                                    uint16_t attacker, uint16_t target);
uint8_t  opeu_battle_element_range(uint8_t* ds, BattleService service,
                                   uint16_t element);
uint8_t  opeu_battle_element_shots(uint8_t* ds, BattleService service,
                                   uint16_t attacker, uint16_t target);
uint8_t  opeu_battle_cell_terrain(uint8_t* ds, BattleService service, uint8_t x,
                                  uint8_t y);
uint16_t opeu_battle_can_enter(uint8_t* ds, BattleService service, uint8_t x,
                               uint8_t y, uint16_t element);
uint8_t  opeu_battle_move_cost(uint8_t* ds, BattleService service, uint8_t x,
                               uint8_t y, uint16_t element);
void opeu_battle_step_unchecked(uint8_t* ds, BattleService service, uint8_t* x,
                                uint8_t* y, uint8_t direction);
uint16_t opeu_battle_step(uint8_t* ds, BattleService service, uint8_t* x,
                          uint8_t* y, uint8_t direction);
uint16_t opeu_battle_unit_controlled(uint8_t* ds, BattleService service,
                                     uint16_t unit);
uint16_t opeu_battle_element_at(uint8_t* ds, BattleService service, uint8_t x,
                                uint8_t y);

// The services of this module.
uint16_t svc_btlcalc(uint8_t* ds, uint16_t id, uint16_t arg);
