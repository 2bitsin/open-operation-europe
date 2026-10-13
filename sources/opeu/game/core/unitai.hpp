// The computer side's orders for its current unit: paths, supply runs, tasks and attacks.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_path_descend(Game& game, uint8_t* x, uint8_t* y, uint8_t tx, uint8_t ty);
uint16_t opeu_unit_route_order(Game& game);
uint16_t opeu_plan_supply(Game& game);
uint16_t opeu_unit_resupply_order(Game& game);
uint16_t opeu_unit_task_order(Game& game);
uint16_t opeu_unit_attack_order(Game& game);
uint16_t opeu_unit_combat_order(Game& game);
void     opeu_unit_task_service(Game& game);
