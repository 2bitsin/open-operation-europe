// The information menu, the weapons views, the victory conditions, the view menu, and the information views'
// shared services.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_view_info_menu(Game& game);
void     opeu_view_weapon_list(Game& game, uint16_t end);
void     opeu_view_own_weapons(Game& game);
void     opeu_view_enemy_weapons(Game& game);
void     opeu_view_arms_menu(Game& game);
void     opeu_view_victory(Game& game);
uint16_t opeu_view_menu_loop(Game& game);
