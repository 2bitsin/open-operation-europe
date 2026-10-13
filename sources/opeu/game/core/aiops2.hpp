// The computer side's supply planning: the neediest objective or unit and the airlift that supplies it.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_ai_neediest_objective(Game& game);
uint16_t opeu_ai_neediest_unit(Game& game);
uint16_t opeu_ai_079c(uint16_t amount);
uint16_t opeu_ai_plan_supply(Game& game);
uint16_t opeu_ai_daytime_plans(Game& game);
