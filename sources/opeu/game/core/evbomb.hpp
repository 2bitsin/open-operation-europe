// The air-raid event of scenario 3.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_bomb_draw_frame(Game& game, uint16_t load, uint16_t x, uint16_t y);
void opeu_bomb_restore_frame(Game& game, uint16_t load, uint16_t x, uint16_t y);
void opeu_bomb_animate(Game& game, uint16_t load, uint16_t unit, uint8_t side);
void opeu_bomb_present(Game& game);
void opeu_bomb_event(Game& game, uint16_t load);
