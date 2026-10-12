// A general's nation, staff role and division assignment, and the name of a nation mask.
#pragma once

#include "types.hpp"

uint16_t opeu_first_mask_name(const uint8_t* ds, uint8_t mask);
uint8_t  opeu_person_lacks_side0_flag(const uint8_t* ds, uint16_t person);
void     opeu_person_set_role(uint8_t* ds, uint16_t person, uint8_t value);
void     opeu_person_set_group(uint8_t* ds, uint16_t person, uint8_t value);
