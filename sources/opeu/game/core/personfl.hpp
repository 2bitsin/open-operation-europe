// A general's nation, staff role and division assignment, and the name of a nation mask.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_first_mask_name(Game const& game, uint8_t mask);
uint8_t  opeu_person_lacks_side0_flag(Game const& game, uint16_t person);
void     opeu_person_set_role(Game& game, uint16_t person, uint8_t value);
void     opeu_person_set_group(Game& game, uint16_t person, uint8_t value);
