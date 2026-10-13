// A regiment's order slots: clear, set, restore and the destination of the current order.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_clear_unit_orders(Game& game, uint16_t unit, uint8_t slots);
void     opeu_set_unit_order(Game& game, uint16_t unit, uint16_t target, uint8_t x, uint16_t save_current, uint8_t code, uint8_t y);
void     opeu_restore_unit_order(Game& game, uint16_t unit);
uint16_t opeu_unit_order_destination(Game const& game, uint16_t unit, uint8_t* x, uint8_t* y);
