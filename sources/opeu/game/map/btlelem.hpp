#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_draw_element_at(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint16_t element, uint8_t mode);
void opeu_battle_redraw_cell(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint16_t element, uint8_t mode);
void opeu_battle_refresh_cell(Game& game, BattleEvent event, uint8_t x, uint8_t y);
void opeu_battle_draw_element_frame(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint16_t offset, uint8_t frame_index, uint16_t element);
void opeu_battle_animate_element(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint16_t turn, uint16_t pause, uint16_t offset, uint16_t element);

// The services of this module.
uint16_t svc_btlelem(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
