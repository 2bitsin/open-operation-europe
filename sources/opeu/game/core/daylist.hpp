// Dated records that are due, and an objective's level.
#pragma once

#include "types.hpp"

uint8_t opeu_list_due_records(uint8_t* ds, uint16_t list, uint8_t mask, uint8_t kind);
uint8_t opeu_objective_level(const uint8_t* ds, uint16_t objective);
