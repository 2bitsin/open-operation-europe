// Task planning of the computer side's divisions: objective and quadrant tasks.
#pragma once

#include "types.hpp"

#include "common/data-segment.hpp"

void     opeu_group_center(uint8_t* ds, uint8_t* x, uint8_t* y, uint8_t group);
uint16_t opeu_tasks_retained(opeu::game::Task const* current, opeu::game::Task const* saved);
uint8_t  opeu_assign_objective_tasks(uint8_t* ds, uint8_t count);
uint8_t  opeu_assign_quadrant_task(uint8_t* ds, uint8_t count);
void     opeu_group_task_service(uint8_t* ds);
