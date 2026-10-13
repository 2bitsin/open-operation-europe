// The current side's ranked objective list and the computer side's retargeting when it changes.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_rank_entry_insert(Game& game, uint8_t side, uint8_t first, uint8_t last, uint16_t objective, uint8_t descending);
void opeu_rank_side_entries(Game& game);
void opeu_side_groups_retarget(Game& game, uint8_t side);
void opeu_turn_selected_begin(Game& game);
