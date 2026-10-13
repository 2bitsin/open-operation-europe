// The supply-transfer panel: keypad, input, the transfer table, the title, the rows and the computer's keys.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_transfer_keypad_hit(Game& game, uint16_t x, uint16_t y);
uint8_t opeu_transfer_input(Game& game, uint16_t row);
void    opeu_transfer_draw(Game& game, uint16_t first, uint16_t second);
void    opeu_transfer_draw_row(Game& game, uint8_t row);
uint8_t opeu_transfer_auto_key(Game& game, uint8_t row);
