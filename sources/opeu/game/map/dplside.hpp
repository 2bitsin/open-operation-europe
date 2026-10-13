#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_deploy_side(Game& game, BattleEvent event, uint8_t side);
void opeu_deploy_battle(Game& game, BattleEvent event);

// The services of this module.
uint16_t svc_dplside(Game& game, uint32_t service, const uint16_t* args, unsigned int count);
