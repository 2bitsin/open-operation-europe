#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_spend_fuel(Game& game, BattleEvent event, uint16_t element, uint16_t before);
void opeu_battle_spend_ammo(Game& game, BattleEvent event, uint16_t element, uint16_t before);
void opeu_battle_spend_stores(Game& game, BattleEvent event, uint16_t element, uint16_t before);

// The services of this module.
uint16_t svc_btlspend(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
