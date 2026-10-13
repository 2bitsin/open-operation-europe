#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_restore_tile(Game& game, uint8_t column, uint8_t row);
void opeu_battle_info_clear(Game& game);
void opeu_battle_panel_begin(Game& game);
void opeu_battle_panel_side1(Game& game);
void opeu_battle_panel_side0(Game& game);
void opeu_battle_name_clear_left(Game& game);
void opeu_battle_name_clear_right(Game& game);
void opeu_battle_commander_panel(Game& game, uint8_t side, uint16_t refresh);
void opeu_battle_commander_wait(Game& game, uint16_t text, uint8_t side, uint8_t value);
void opeu_battle_commander_message(Game& game, uint16_t text, uint8_t side, uint8_t value);
void opeu_battle_element_destroy(Game& game, uint16_t element);
