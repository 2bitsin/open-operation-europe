// The vacant command posts of a side: its three staff roles and its four divisions.
#pragma once

#include "types.hpp"

uint8_t opeu_find_vacant_post(const uint8_t* ds);
void    opeu_clear_staff_role(uint8_t* ds, uint8_t side, uint8_t kind);
void    opeu_clear_division_command(uint8_t* ds, uint8_t side, uint8_t group);
