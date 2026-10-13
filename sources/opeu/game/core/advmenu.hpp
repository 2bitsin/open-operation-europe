// Staff advice menus.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_control_prepare(Game& game);
void opeu_staff_advice_menu(Game& game, HostEvent event);
void opeu_turn_action_5(Game& game, HostEvent event);

// The services of this module as direct calls.
uint16_t svc_advmenu(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
