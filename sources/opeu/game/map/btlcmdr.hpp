#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_restore_tile(Game& game, BattleEvent event, uint8_t column, uint8_t row);
void opeu_battle_info_clear(Game& game, BattleEvent event);
void opeu_battle_panel_begin(Game& game, BattleEvent event);
void opeu_battle_panel_side1(Game& game, BattleEvent event);
void opeu_battle_panel_side0(Game& game, BattleEvent event);
void opeu_battle_name_clear_left(Game& game, BattleEvent event);
void opeu_battle_name_clear_right(Game& game, BattleEvent event);
void opeu_battle_commander_panel(Game& game, BattleEvent event, uint8_t side, uint16_t refresh);
void opeu_battle_commander_wait(Game& game, BattleEvent event, uint16_t text, uint8_t side, uint8_t value);
void opeu_battle_commander_message(Game& game, BattleEvent event, uint16_t text, uint8_t side, uint8_t value);
void opeu_battle_element_destroy(Game& game, BattleEvent event, uint16_t element);

// The services of this module.
uint16_t svc_battle_commander(Game& game, uint32_t address, const uint16_t* args, unsigned count);
uint16_t svc_btlcmdr(Game& game, uint32_t service, const uint16_t* args, unsigned int count);
