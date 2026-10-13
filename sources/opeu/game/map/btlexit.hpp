#pragma once

#include "common/events.hpp"
#include "map/grid-cell.hpp"
#include "types.hpp"

[[nodiscard]] auto opeu_battle_exit_ok(Game& game, opeu::game::GridCell cell, uint16_t element) -> uint16_t;
uint16_t           opeu_battle_cursor_exit_ok(Game& game, uint16_t element);
