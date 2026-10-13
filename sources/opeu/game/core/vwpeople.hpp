// The generals views: own and enemy generals and their menu.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_view_people_list(Game& game, uint8_t count);
void opeu_view_own_generals(Game& game);
void opeu_view_enemy_generals(Game& game);
void opeu_view_generals_menu(Game& game);
