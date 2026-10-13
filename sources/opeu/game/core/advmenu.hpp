// Staff advice menus.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_control_prepare(uint8_t* ds, HostEvent event);
void opeu_staff_advice_menu(uint8_t* ds, HostEvent event);
void opeu_turn_action_5(uint8_t* ds, HostEvent event);

// The services of this module as direct calls.
uint16_t svc_advmenu(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
