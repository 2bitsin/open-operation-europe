// The computer side's supply budget: how many 3000-unit loads of a resource it takes from its stock.
#pragma once

#include "types.hpp"

uint16_t opeu_ai_supply_share(uint8_t* ds, uint16_t* budget, uint16_t amount, uint8_t which);
