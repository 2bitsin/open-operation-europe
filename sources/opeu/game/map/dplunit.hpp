#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_deploy_unit(Game& game, BattleEvent event, uint16_t unit);

// The services of this module.
uint16_t svc_dplunit(Game& game, uint32_t service, const uint16_t* args, unsigned int count);
