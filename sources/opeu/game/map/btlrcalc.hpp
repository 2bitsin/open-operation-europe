#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_battle_type_skill(Game& game, BattleEvent event, uint16_t element);
uint16_t opeu_battle_ranged_hits(Game& game, BattleEvent event, uint16_t shooter, uint16_t target);
uint16_t opeu_battle_ranged_power(Game& game, BattleEvent event, uint16_t shooter, uint16_t target);
uint16_t opeu_battle_ranged_defense(Game& game, BattleEvent event, uint16_t element);
// One ranged shot: its casualties, and whether it was a fixed barrage into a bunker (or at no target).
struct RangedShot {
  uint16_t losses;
  bool     into_bunker;
};
auto opeu_battle_ranged_damage(Game& game, BattleEvent event, uint16_t shooter, uint16_t target) -> RangedShot;

// The services of this module.
uint16_t svc_btlrcalc(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
