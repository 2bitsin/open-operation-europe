// The computer side's division tasks.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_clean_group_tasks(Game& game);
uint16_t opeu_side_has_entries(Game& game);
uint16_t opeu_group_has_tasks(Game& game);
void     opeu_turn_group_action(Game& game, uint16_t (*event)(Game&, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t));
uint16_t opeu_turn_final_predicate(Game& game, uint16_t (*event)(Game&, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t));

// Runs the services the module's routines call through their event.
uint16_t svc_grptask(Game& game, uint16_t id, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3);
