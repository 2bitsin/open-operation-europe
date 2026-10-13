#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_sprite_offset(Game& game, BattleEvent event, uint8_t sprite);
void     opeu_battle_draw_sprite(Game& game, BattleEvent event, uint16_t offset);
void     opeu_battle_draw_extra(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint8_t code);
void     opeu_battle_draw_terrain(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint8_t code);
uint16_t opeu_battle_sprite_valid(Game& game, BattleEvent event, uint8_t sprite);
void     opeu_battle_draw_neighbors(Game& game, BattleEvent event, uint8_t x, uint8_t y);

// The services of this module.
uint16_t svc_btlsprt(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
