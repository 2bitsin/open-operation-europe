#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_battle_element_icon(Game& game, uint16_t element);
uint16_t opeu_battle_element_visible(Game& game, uint16_t element);
void     opeu_battle_draw_count(Game& game, uint16_t count, uint8_t mode, uint16_t side);
uint16_t opeu_battle_code_low(Game& game, uint8_t code);
void     opeu_battle_draw_icon(Game& game, uint16_t element, uint8_t side, uint8_t mode);
