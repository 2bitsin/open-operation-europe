// The computer side's division tasks.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_clean_group_tasks(Game& game);
uint16_t opeu_side_has_entries(Game& game);
uint16_t opeu_group_has_tasks(Game& game);
void     opeu_turn_group_action(Game& game);
uint16_t opeu_turn_final_predicate(Game& game);
