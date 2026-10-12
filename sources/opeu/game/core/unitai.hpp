// The computer side's orders for its current unit: paths, supply runs, tasks and attacks.
#pragma once

#include "types.hpp"

uint16_t opeu_path_descend(uint8_t* ds, uint8_t* x, uint8_t* y, uint8_t tx, uint8_t ty);
uint16_t opeu_unit_route_order(uint8_t* ds);
uint16_t opeu_plan_supply(uint8_t* ds);
uint16_t opeu_unit_resupply_order(uint8_t* ds);
uint16_t opeu_unit_task_order(uint8_t* ds);
uint16_t opeu_unit_attack_order(uint8_t* ds);
uint16_t opeu_unit_combat_order(uint8_t* ds);
void     opeu_unit_task_service(uint8_t* ds);
