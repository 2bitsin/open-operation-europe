// The supply-allocation panel: its keypad, input, table access, title and digits.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

#include <cstdint>

uint8_t  opeu_supply_keypad_hit(Game& game, uint16_t x, uint16_t y);
uint8_t  opeu_supply_input(Game& game, uint16_t row);
void     opeu_supply_set(Game& game, uint8_t side, uint8_t row, uint8_t column, uint16_t value);
uint16_t opeu_supply_get(Game const& game, uint8_t side, uint8_t row, uint8_t column);
void     opeu_supply_draw(Game& game, uint16_t first, uint16_t second);
void     opeu_supply_digits(Game& game, uint16_t x, uint16_t y, uint16_t value);
