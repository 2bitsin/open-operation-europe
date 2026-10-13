// The regiments views: own divisions and their regiments, sighted enemy regiments, and a regiment's battalions.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void    opeu_view_regiment_list(Game& game);
uint8_t opeu_list_group_units(Game& game, uint8_t group);
void    opeu_view_own_divisions(Game& game);
void    opeu_view_enemy_units(Game& game);
