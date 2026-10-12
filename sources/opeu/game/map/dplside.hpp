#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_deploy_side(uint8_t* ds, BattleEvent event, uint8_t side);
void opeu_deploy_battle(uint8_t* ds, BattleEvent event);

// The services of this module.
uint16_t svc_dplside(uint8_t* ds, uint32_t service, const uint16_t* args,
                     unsigned int count);
