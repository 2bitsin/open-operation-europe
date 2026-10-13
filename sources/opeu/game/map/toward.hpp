#pragma once

#include "map/grid-cell.hpp"
#include "map/path-grid.hpp"
#include "types.hpp"

// One step of a map cell toward a goal along the path grid's cheapest neighbour, and that neighbour's cost (0xffff, the
// cell itself, when every neighbour is off the map).
[[nodiscard]] auto opeu_grid_step_toward(opeu::game::PathGrid const& path, opeu::game::GridCell from, opeu::game::GridCell goal) -> opeu::game::PathStep;
