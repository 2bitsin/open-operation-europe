#pragma once

#include "common/events.hpp"
#include "common/grid-cell.hpp"
#include "types.hpp"

namespace opeu::game::detail::movetgt {

// The objective or regiment a movement order leads to, and its cell; `near` is 0 when the order has none left.
struct OrderTarget {
  std::uint16_t near;
  GridCell      cell;
};
} // namespace opeu::game::detail::movetgt

namespace opeu::game {
using detail::movetgt::OrderTarget;
} // namespace opeu::game

uint16_t opeu_move_adjacent(Game& game, uint8_t x, uint8_t y);
auto     opeu_move_objective_target(Game& game, uint8_t side) -> opeu::game::OrderTarget;
auto     opeu_move_unit_target(Game& game) -> opeu::game::OrderTarget;
uint16_t opeu_move_level_raise(Game& game, uint16_t record, uint8_t value);
uint16_t opeu_move_level_lower(Game& game, uint16_t record, uint8_t value);
