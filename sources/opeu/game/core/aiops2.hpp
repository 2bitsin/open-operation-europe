// The computer side's supply planning: the neediest objective or unit and the airlift that supplies it.
#pragma once

#include "types.hpp"

uint16_t opeu_ai_neediest_objective(uint8_t* ds);
uint16_t opeu_ai_neediest_unit(uint8_t* ds);
uint16_t opeu_ai_079c(uint16_t amount);
uint16_t opeu_ai_plan_supply(uint8_t* ds);
uint16_t opeu_ai_daytime_plans(uint8_t* ds);
