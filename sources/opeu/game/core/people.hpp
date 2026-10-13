// Finding a side's general by staff role or by division.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_find_side_record(Game const& game, uint8_t side, uint8_t kind);
uint16_t opeu_find_group_record(Game const& game, uint8_t side, uint8_t group);
