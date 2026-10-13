#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_grid_step_toward(Game& game, const uint8_t* far_data, uint16_t x, uint16_t y, uint8_t tx, uint8_t ty);
