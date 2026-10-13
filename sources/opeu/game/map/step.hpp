#pragma once

#include "common/grid-cell.hpp"
#include "types.hpp"

// The cell one step from a cell in one of the eight directions, without a bounds check.
[[nodiscard]] auto opeu_grid_step_unchecked(opeu::game::GridCell cell, std::uint8_t direction) -> opeu::game::GridCell;
