// The computer side's supply budget: how many 3000-unit loads of a resource it takes from its stock.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_ai_supply_share(Game& game, uint16_t* budget, uint16_t amount, uint8_t which);
