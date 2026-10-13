#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_cell_open(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint16_t element);
void     opeu_battle_cursor_draw(Game& game, BattleEvent event);
void     opeu_battle_cursor_step(Game& game, BattleEvent event, int16_t dx, int16_t dy);
void     opeu_battle_mouse_cell(Game& game, BattleEvent event, uint16_t x_at, uint16_t y_at);

// The services of this module.
uint16_t svc_btlcurs(Game& game, uint32_t service, const uint16_t* args, unsigned int count);
