#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_battle_dispatch(Game& game, BattleEvent event, uint8_t handler);
uint16_t opeu_battle_cell_row(Game& game, BattleEvent event, uint8_t x, uint8_t y);
uint16_t opeu_battle_cell_column(Game& game, BattleEvent event, uint8_t x, uint8_t y);
void     opeu_battle_draw_cursor(Game& game, BattleEvent event, uint8_t x, uint8_t y);
void     opeu_battle_draw_frame(Game& game, BattleEvent event, uint8_t x, uint8_t y);
void     opeu_battle_draw_number(Game& game, BattleEvent event, uint16_t value);

// The services of this module.
uint16_t svc_btldraw(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
