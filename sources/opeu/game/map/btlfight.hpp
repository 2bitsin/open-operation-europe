#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_random_target(Game& game, BattleEvent event, uint16_t element);
void     opeu_battle_element_turn(Game& game, BattleEvent event, uint16_t element);
uint16_t opeu_battle_unit_fight(Game& game, BattleEvent event, uint16_t unit);
uint16_t opeu_battle_round(Game& game, BattleEvent event);
uint16_t opeu_battle_rounds(Game& game, BattleEvent event);
void     opeu_battle_finish_turn(Game& game, BattleEvent event);
void     opeu_battle_finish_none(Game& game, BattleEvent event);

// The services of this module.
uint16_t svc_btlfight(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
