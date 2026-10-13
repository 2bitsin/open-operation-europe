// The computer side's targets and staff: objective capacities, the side's attack target, and the rotation of unfit
// generals.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_ai_refresh_capacities(Game& game);
uint16_t opeu_ai_pick_enemy_objective(Game& game);
uint16_t opeu_ai_pick_enemy_unit(Game& game);
uint16_t opeu_ai_choose_target(Game& game);
uint16_t opeu_ai_rotate_staff(Game& game);
