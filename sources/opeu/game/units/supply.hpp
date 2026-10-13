// The supply exchange between two units or a unit and an objective.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_supply_load_unit(Game& game, uint16_t unit, uint8_t side);
void opeu_supply_load_objective(Game& game, uint16_t objective);
void opeu_supply_exchange_units(Game& game, uint16_t first, uint16_t second);
void opeu_supply_exchange_objective(Game& game, uint16_t unit, uint16_t objective);
