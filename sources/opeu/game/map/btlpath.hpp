#pragma once

#include "common/events.hpp"
#include "common/grid-cell.hpp"
#include "types.hpp"

#include <cstdint>

namespace opeu::game::detail::btlpath {

// A step downhill in battle_path_costs: the cheapest neighbour of a cell and its cost (0xffff, and the cell itself,
// when no neighbour has been reached by the flood).
[[nodiscard]] auto DownhillStep(Game const& game, GridCell from) -> PathStep;
} // namespace opeu::game::detail::btlpath

namespace opeu::game {
using detail::btlpath::DownhillStep;
} // namespace opeu::game

[[nodiscard]] auto opeu_battle_path_cost(Game& game, opeu::game::GridCell cell, uint16_t element) -> uint8_t;
[[nodiscard]] auto opeu_battle_path_cost_any(Game& game, opeu::game::GridCell cell, uint16_t element) -> uint8_t;
// DownhillStep through the cell's bytes at the DS offsets x and y; with no reached neighbour the cell takes stale DS
// bytes. For the battle bench.
uint16_t opeu_battle_path_step(Game& game, uint16_t x, uint16_t y);
uint16_t opeu_battle_path_flood(Game& game, uint8_t x, uint8_t y, uint8_t to_x, uint8_t to_y, uint16_t element, uint8_t mode);
void     opeu_battle_report_sighted(Game& game, uint16_t element);
uint16_t opeu_battle_spot_chance(Game& game, uint16_t element, uint16_t other);
void     opeu_battle_spot_enemies(Game& game, uint16_t element);
