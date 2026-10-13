// The air-operation display: the plane sprite and the weather report before an operation.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void    opeu_air_sprite_mirror(Game& game, uint16_t image, uint8_t keep);
void    opeu_air_sprite_draw(Game& game, uint16_t load, uint16_t x, uint16_t y, uint8_t keep);
void    opeu_air_sprite_restore(Game& game, uint16_t load, uint16_t x, uint16_t y);
uint8_t opeu_air_report(Game& game, uint16_t what, uint8_t wait);
