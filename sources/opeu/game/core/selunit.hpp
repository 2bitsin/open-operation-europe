// The selected unit of a division or of a side.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_find_group_selected_unit(Game const& game, uint16_t group);
uint16_t opeu_find_side_selected_unit(Game const& game, uint8_t side);
