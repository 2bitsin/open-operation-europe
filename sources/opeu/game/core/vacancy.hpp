// The vacant command posts of a side: its three staff roles and its four divisions.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_find_vacant_post(Game const& game);
void    opeu_clear_staff_role(Game& game, uint8_t side, uint8_t kind);
void    opeu_clear_division_command(Game& game, uint8_t side, uint8_t group);
