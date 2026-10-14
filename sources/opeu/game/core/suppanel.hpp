// The supply-allocation panel: its keypad, input, table access, title and digits.
#pragma once

#include "common/events.hpp"
#include "common/exchange-panel.hpp"
#include "common/text.hpp"
#include "types.hpp"

#include <cstdint>

uint8_t  opeu_supply_keypad_hit(Game& game, uint16_t x, uint16_t y);
auto     opeu_supply_input(Game& game) -> opeu::game::PanelInput;
void     opeu_supply_set(Game& game, uint8_t side, uint8_t row, uint8_t column, uint16_t value);
uint16_t opeu_supply_get(Game const& game, uint8_t side, uint8_t row, uint8_t column);
void     opeu_supply_draw(Game& game, Text first, Text second);
