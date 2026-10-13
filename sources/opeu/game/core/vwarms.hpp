// The information menu, the weapons views, the victory conditions, the view menu, and the information views'
// shared services.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_view_info_menu(uint8_t* ds, HostEvent event);
void     opeu_view_weapon_list(uint8_t* ds, HostEvent event, uint16_t end);
void     opeu_view_own_weapons(uint8_t* ds, HostEvent event);
void     opeu_view_enemy_weapons(uint8_t* ds, HostEvent event);
void     opeu_view_arms_menu(uint8_t* ds, HostEvent event);
void     opeu_view_victory(uint8_t* ds, HostEvent event);
uint16_t opeu_view_menu_loop(uint8_t* ds, HostEvent event);

// The services every information view calls (common/view-service.hpp), as direct calls; module names the caller in
// the report of an unknown service.
uint16_t svc_view(uint8_t* ds, const char* module, uint16_t id, const uint16_t* args, uint16_t count);
// The services of the views above.
uint16_t svc_vwarms(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
