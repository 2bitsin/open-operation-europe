// The computer side's division tasks.
#pragma once

#include "types.hpp"

void     opeu_clean_group_tasks(uint8_t* ds);
uint16_t opeu_side_has_entries(uint8_t* ds);
uint16_t opeu_group_has_tasks(uint8_t* ds);
void opeu_turn_group_action(uint8_t* ds, uint16_t (*event)(uint8_t*, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t));
uint16_t opeu_turn_final_predicate(uint8_t* ds,
                                   uint16_t (*event)(uint8_t*, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t));

// Runs the services the module's routines call through their event.
uint16_t svc_grptask(uint8_t* ds, uint16_t id, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3);
