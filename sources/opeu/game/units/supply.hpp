// The supply exchange between two units or a unit and an objective.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_supply_load_unit(Game& game, uint16_t unit, uint8_t side);
void opeu_supply_load_objective(Game& game, uint16_t objective);
void opeu_supply_exchange_units(Game& game, uint16_t (*event)(Game&, uint16_t, const uint16_t*, uint16_t), uint16_t first, uint16_t second);
void opeu_supply_exchange_objective(Game& game, uint16_t (*event)(Game&, uint16_t, const uint16_t*, uint16_t), uint16_t unit, uint16_t objective);

// The services the module calls out to.
uint16_t svc_supply(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
