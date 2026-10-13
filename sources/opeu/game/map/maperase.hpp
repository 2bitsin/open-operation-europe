#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_map_erase(Game& game, uint8_t x, uint8_t y);
void opeu_arrival_prepare(Game& game, uint16_t unit);
void opeu_unit_remove_draw(Game& game, uint16_t unit);
void opeu_daily_activate(Game& game, uint8_t side);
