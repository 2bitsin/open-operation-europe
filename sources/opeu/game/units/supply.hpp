// The supply exchange between two units or a unit and an objective.
#pragma once

#include "types.hpp"

void opeu_supply_load_unit(uint8_t* ds, uint16_t unit, uint8_t side);
void opeu_supply_load_objective(uint8_t* ds, uint16_t objective);
void opeu_supply_exchange_units(uint8_t* ds, uint16_t (*event)(uint8_t*, uint16_t, const uint16_t*, uint16_t),
                                uint16_t first, uint16_t second, uint16_t a, uint16_t b);
void opeu_supply_exchange_objective(uint8_t* ds, uint16_t (*event)(uint8_t*, uint16_t, const uint16_t*, uint16_t),
                                    uint16_t unit, uint16_t objective, uint16_t a);

// The services the module calls out to.
uint16_t svc_supply(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
