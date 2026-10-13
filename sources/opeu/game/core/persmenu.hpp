// The personnel orders menu (turn action 4) and its list and picture callbacks.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void    opeu_personnel_finish(Game& game, uint16_t segment);
uint8_t opeu_commander_always(Game& game, uint16_t segment, uint8_t n);
void    opeu_rest_thanks(Game& game, uint16_t segment, uint16_t person);
void    opeu_assign_confirm(Game& game, uint16_t segment, uint16_t person);
uint8_t opeu_division_toggle(Game& game, uint16_t segment, uint8_t n);
void    opeu_division_commander_menu(Game& game, uint16_t segment);
uint8_t opeu_idle_commanders_fill(Game& game, uint16_t segment);
void    opeu_commander_assign(Game& game, uint16_t segment);
void    opeu_commander_rest(Game& game, uint16_t segment);
void    opeu_turn_action_4(Game& game, uint16_t segment);
