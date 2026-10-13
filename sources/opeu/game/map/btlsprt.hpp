#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_sprite_offset(Game& game, uint8_t sprite);
void     opeu_battle_draw_sprite(Game& game, uint16_t offset);
void     opeu_battle_draw_extra(Game& game, uint8_t x, uint8_t y, uint8_t code);
void     opeu_battle_draw_terrain(Game& game, uint8_t x, uint8_t y, uint8_t code);
uint16_t opeu_battle_sprite_valid(Game& game, uint8_t sprite);
void     opeu_battle_draw_neighbors(Game& game, uint8_t x, uint8_t y);
