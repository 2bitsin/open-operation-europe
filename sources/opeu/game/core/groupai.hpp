// Task planning of the computer side's divisions: objective and quadrant tasks.
#pragma once

#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/events.hpp"

void     opeu_group_center(Game& game, uint8_t* x, uint8_t* y, uint8_t group);
uint16_t opeu_tasks_retained(opeu::game::Task const* current, opeu::game::Task const* saved);
uint8_t  opeu_assign_objective_tasks(Game& game, uint8_t count);
uint8_t  opeu_assign_quadrant_task(Game& game, uint8_t count);
void     opeu_group_task_service(Game& game);
