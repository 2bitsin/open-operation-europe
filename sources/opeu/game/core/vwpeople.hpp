// The generals views: own and enemy generals and their menu.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_view_people_list(Game& game, HostEvent event, uint8_t count);
void opeu_view_own_generals(Game& game, HostEvent event);
void opeu_view_enemy_generals(Game& game, HostEvent event);
void opeu_view_generals_menu(Game& game, HostEvent event);

// The services of the generals views (the information views' shared services).
uint16_t svc_vwpeople(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
