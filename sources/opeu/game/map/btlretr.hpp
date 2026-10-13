#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_retreat_cell_ok(Game& game, uint8_t x, uint8_t y, uint16_t unit);
void     opeu_battle_retreat_to(Game& game, uint16_t unit, uint8_t x, uint8_t y);
void     opeu_battle_unit_retreat(Game& game, uint16_t unit, uint8_t side);
void     opeu_battle_morale_check(Game& game);
