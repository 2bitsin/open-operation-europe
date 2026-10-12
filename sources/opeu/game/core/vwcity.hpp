// The cities views: own and enemy cities, their stocks, and the cities menu.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_view_city_list(uint8_t* ds, HostEvent event, uint16_t sp, uint8_t count);
void opeu_view_own_cities(uint8_t* ds, HostEvent event, uint16_t sp);
void opeu_view_enemy_cities(uint8_t* ds, HostEvent event, uint16_t sp);
void opeu_view_cities_menu(uint8_t* ds, HostEvent event, uint16_t sp);

// The services of the cities views (the information views' shared services).
uint16_t svc_vwcity(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
