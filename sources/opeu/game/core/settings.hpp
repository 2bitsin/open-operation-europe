// The Settings command: options, saving, a new game and leaving to DOS.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_settings_print(Game& game);
void     opeu_settings_battles(Game& game);
void     opeu_settings_delay(Game& game);
void     opeu_settings_map(Game& game);
void     opeu_settings_sound(Game& game);
void     opeu_settings_pictures(Game& game);
void     opeu_save_rebase(Game& game);
void     opeu_save_unrebase(Game& game);
void     opeu_save_checksum(Game& game, uint8_t slot);
void     opeu_save_create(Game& game);
uint16_t opeu_save_prompt_box(Game& game, uint16_t x, uint16_t y, uint16_t text);
void     opeu_save_game(Game& game);
void     opeu_quit_to_dos(Game& game);
void     opeu_settings_leave(Game& game);
void     opeu_settings_new_game(Game& game);
uint16_t opeu_settings_save_menu(Game& game);
void     opeu_turn_action_10(Game& game);
