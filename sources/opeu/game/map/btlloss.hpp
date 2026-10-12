#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_loss_check(uint8_t* ds, BattleEvent event, uint16_t attacker, uint16_t target);

// The services of this module.
uint16_t svc_btlloss(uint8_t* ds, uint32_t service, const uint16_t* args, unsigned int count);
