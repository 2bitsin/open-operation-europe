#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_battle_mine_report(Game& game, BattleEvent event, uint16_t element);
uint16_t opeu_battle_mine_damage(Game& game, BattleEvent event, uint16_t element);
void     opeu_battle_mine_hit(Game& game, BattleEvent event, uint16_t element);

// The services of this module.
uint16_t svc_battle_mines(Game& game, uint32_t address, const uint16_t* args, unsigned count);
uint16_t svc_btlmines(Game& game, uint32_t service, const uint16_t* args, unsigned int count);
