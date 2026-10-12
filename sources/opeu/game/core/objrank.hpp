// The current side's ranked objective list and the computer side's retargeting when it changes.
#pragma once

#include "types.hpp"

uint8_t opeu_objective_rank_level(uint8_t* ds, uint16_t objective);
void    opeu_rank_entry_insert(uint8_t* ds, uint8_t side, uint8_t first, uint8_t last, uint16_t objective,
                               uint8_t descending);
void    opeu_rank_side_entries(uint8_t* ds);
void    opeu_side_groups_retarget(uint8_t* ds, uint8_t side);
void    opeu_turn_selected_begin(uint8_t* ds);
