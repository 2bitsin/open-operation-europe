#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_cell_open(uint8_t* ds, BattleEvent event, uint8_t x, uint8_t y, uint16_t element);
void     opeu_battle_cursor_draw(uint8_t* ds, BattleEvent event);
void     opeu_battle_cursor_step(uint8_t* ds, BattleEvent event, int16_t dx, int16_t dy);
void     opeu_battle_mouse_cell(uint8_t* ds, BattleEvent event, uint16_t x_at, uint16_t y_at);

// The services of this module.
uint16_t svc_btlcurs(uint8_t* ds, uint32_t service, const uint16_t* args, unsigned int count);
