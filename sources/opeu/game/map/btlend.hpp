#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_run(Game& game, BattleEvent event);
void opeu_battle_refresh_strengths(Game& game, BattleEvent event);
void opeu_battle_init_element(Game& game, BattleEvent event, uint16_t element, uint8_t side, uint8_t index);
void opeu_battle_init_elements(Game& game, BattleEvent event);
void opeu_battle_objective_damage(Game& game, BattleEvent event);
void opeu_notice_other(Game& game, BattleEvent event);

// The services of this module.
uint16_t svc_btlend(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
