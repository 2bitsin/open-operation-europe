// The cities views: own and enemy cities, their stocks, and the cities menu.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_view_city_list(Game& game, uint8_t count);
void opeu_view_own_cities(Game& game);
void opeu_view_enemy_cities(Game& game);
void opeu_view_cities_menu(Game& game);
