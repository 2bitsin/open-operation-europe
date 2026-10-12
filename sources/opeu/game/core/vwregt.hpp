// The regiments views: own divisions and their regiments, sighted enemy regiments, and a regiment's battalions.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void    opeu_view_regiment_list(uint8_t* ds, HostEvent event, uint16_t sp);
uint8_t opeu_list_group_units(uint8_t* ds, HostEvent event, uint16_t sp, uint8_t group);
void    opeu_view_own_divisions(uint8_t* ds, HostEvent event, uint16_t sp);
void    opeu_view_enemy_units(uint8_t* ds, HostEvent event, uint16_t sp);

// The services of the regiments views (the information views' shared services).
uint16_t svc_vwregt(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
