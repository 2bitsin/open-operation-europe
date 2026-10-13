#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_spend_fuel(Game& game, uint16_t element, uint16_t before);
void opeu_battle_spend_ammo(Game& game, uint16_t element, uint16_t before);
void opeu_battle_spend_stores(Game& game, uint16_t element, uint16_t before);
