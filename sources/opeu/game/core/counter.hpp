// The countdown a general keeps in the high five bits of Person::assignment.
#pragma once

#include "common/data-segment.hpp"
#include "types.hpp"

void    opeu_set_record_counter(opeu::game::Person& general, uint8_t count);
uint8_t opeu_get_record_counter(opeu::game::Person const& general);
