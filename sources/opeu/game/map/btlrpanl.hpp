#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_wiped_notice(Game& game, BattleEvent event, uint16_t element);
void opeu_battle_ranged_panel(Game& game, BattleEvent event, uint16_t shooter, uint16_t target);

// The services of this module.
uint16_t svc_btlrpanl(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
