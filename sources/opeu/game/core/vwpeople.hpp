// The generals views: own and enemy generals and their menu.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_view_people_list(uint8_t* ds, HostEvent event, uint16_t sp,
                           uint8_t count);
void opeu_view_own_generals(uint8_t* ds, HostEvent event, uint16_t sp);
void opeu_view_enemy_generals(uint8_t* ds, HostEvent event, uint16_t sp);
void opeu_view_generals_menu(uint8_t* ds, HostEvent event, uint16_t sp);

// The services of the generals views (the information views' shared services).
uint16_t svc_vwpeople(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count);
