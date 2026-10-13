// The personnel orders menu (turn action 4) and its list and picture callbacks.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void    opeu_personnel_finish(Game& game, HostEvent event, uint16_t segment);
uint8_t opeu_commander_always(Game& game, uint16_t segment, uint8_t n);
void    opeu_rest_thanks(Game& game, HostEvent event, uint16_t segment, uint16_t person);
void    opeu_assign_confirm(Game& game, HostEvent event, uint16_t segment, uint16_t person);
uint8_t opeu_division_toggle(Game& game, HostEvent event, uint16_t segment, uint8_t n);
void    opeu_division_commander_menu(Game& game, HostEvent event, uint16_t segment);
uint8_t opeu_idle_commanders_fill(Game& game, uint16_t segment);
void    opeu_commander_assign(Game& game, HostEvent event, uint16_t segment);
void    opeu_commander_rest(Game& game, HostEvent event, uint16_t segment);
void    opeu_turn_action_4(Game& game, HostEvent event, uint16_t segment);

// The services the personnel routines are called with.
uint16_t svc_persmenu(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
