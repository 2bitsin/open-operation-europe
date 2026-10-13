// Dated records that are due, and an objective's level.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_list_due_records(Game& game, uint16_t list, uint8_t mask, uint8_t kind);
uint8_t opeu_objective_level(Game const& game, uint16_t objective);
