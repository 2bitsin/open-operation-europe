#pragma once

#include "common/events.hpp"
#include "common/grid-cell.hpp"
#include "types.hpp"

// The facing (CellFacing bits) from one battle cell toward another: up or down along a column, else left or right.
[[nodiscard]] auto CellFacingToward(opeu::game::CellMove move) -> uint8_t;
uint8_t            opeu_battle_element_facing(Game& game, uint16_t element, uint16_t other);
uint8_t            opeu_battle_terrain_mode(Game& game, uint16_t element, uint16_t other);
