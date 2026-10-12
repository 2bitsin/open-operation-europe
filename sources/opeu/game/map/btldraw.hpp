#pragma once

#include "types.hpp"
#include "common/events.hpp"

void     opeu_battle_dispatch(uint8_t* ds, BattleEvent event, uint8_t handler);
uint16_t opeu_battle_cell_row(uint8_t* ds, BattleEvent event, uint8_t x,
                              uint8_t y);
uint16_t opeu_battle_cell_column(uint8_t* ds, BattleEvent event, uint8_t x,
                                 uint8_t y);
void     opeu_battle_draw_cursor(uint8_t* ds, BattleEvent event, uint8_t x,
                                 uint8_t y);
void     opeu_battle_draw_frame(uint8_t* ds, BattleEvent event, uint8_t x,
                                uint8_t y);
void opeu_battle_draw_number(uint8_t* ds, BattleEvent event, uint16_t value);

// The services of this module.
uint16_t svc_btldraw(uint8_t* ds, uint32_t address, const uint16_t* args,
                     unsigned int count);
