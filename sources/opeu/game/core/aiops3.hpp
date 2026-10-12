// The computer side's targets and staff: objective capacities, the side's attack target, and the rotation of unfit
// generals.
#pragma once

#include "types.hpp"

uint16_t opeu_ai_refresh_capacities(uint8_t* ds);
uint16_t opeu_ai_pick_enemy_objective(uint8_t* ds);
uint16_t opeu_ai_pick_enemy_unit(uint8_t* ds);
uint16_t opeu_ai_choose_target(uint8_t* ds);
uint16_t opeu_ai_person_merit(uint8_t* ds, uint16_t a);
uint8_t  opeu_ai_rank_people(uint8_t* ds, uint16_t* list);
uint16_t opeu_ai_replace_person(uint8_t* ds, uint16_t person);
uint16_t opeu_ai_rotate_staff(uint8_t* ds);
