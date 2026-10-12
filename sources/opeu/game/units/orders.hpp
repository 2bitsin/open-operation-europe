// A regiment's order slots: clear, set, restore and the destination of the current order.
#pragma once

#include "types.hpp"

void opeu_clear_unit_orders(uint8_t* ds, uint16_t unit, uint8_t slots);
void opeu_set_unit_order(uint8_t* ds, uint16_t unit, uint16_t target, uint8_t x, uint16_t save_current, uint8_t code,
                         uint8_t y);
void opeu_restore_unit_order(uint8_t* ds, uint16_t unit);
uint16_t opeu_unit_order_destination(const uint8_t* ds, uint16_t unit, uint8_t* x, uint8_t* y);
