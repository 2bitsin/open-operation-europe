// Regiment order helpers of the Orders command.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void    opeu_cancel_unit_orders(Game& game, uint16_t unit, uint16_t person);
uint8_t opeu_supply_unit_enabled(Game& game);
void    opeu_order_layout(Game& game, uint8_t kind);
void    opeu_order_layout_pair(Game& game, uint16_t kind);
