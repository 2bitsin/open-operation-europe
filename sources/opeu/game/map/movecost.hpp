#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_move_cost(Game& game, uint8_t x, uint8_t y, uint16_t unit);
void     opeu_move_clear(uint8_t* far_data, uint8_t x, uint8_t y);
uint16_t opeu_move_reachable(Game& game, uint16_t* far_words, uint8_t x, uint8_t y, uint8_t tx, uint8_t ty, uint16_t unit);
