// The supply-transfer panel: keypad, input, the transfer table, the title, the rows and the computer's keys.
#pragma once

#include "common/events.hpp"
#include "common/exchange-panel.hpp"
#include "common/text.hpp"
#include "types.hpp"

uint8_t opeu_transfer_keypad_hit(Game& game, uint16_t x, uint16_t y);
auto    opeu_transfer_input(Game& game) -> opeu::game::PanelInput;
void    opeu_transfer_draw(Game& game, Text first, Text second);
void    opeu_transfer_draw_row(Game& game, uint8_t row);
uint8_t opeu_transfer_auto_key(Game& game, uint8_t row);
