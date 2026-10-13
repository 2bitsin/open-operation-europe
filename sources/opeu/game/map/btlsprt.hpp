#pragma once

#include "common/events.hpp"
#include "map/grid-cell.hpp"
#include "types.hpp"

uint16_t opeu_battle_sprite_offset(Game& game, uint8_t sprite);
void     opeu_battle_draw_sprite(Game& game, uint16_t offset);
auto     opeu_battle_draw_extra(Game& game, opeu::game::GridCell cell, uint8_t code) -> void;
auto     opeu_battle_draw_terrain(Game& game, opeu::game::GridCell cell, uint8_t code) -> void;
uint16_t opeu_battle_sprite_valid(Game& game, uint8_t sprite);
void     opeu_battle_draw_neighbors(Game& game, uint8_t x, uint8_t y);
