// One step from a cell in one of the eight directions, without a bounds check. Directions 0..7 run from the row below
// (left to right), through the left and right neighbours, to the row above; other directions leave the cell as it is.
// Byte arithmetic wraps.
#include "map/step.hpp"
#include "types.hpp"

#include <array>

using namespace opeu::game;

namespace {
constexpr std::array<std::int8_t, 8> STEP_X{ -1, 0, 1, -1, 1, -1, 0, 1 };
constexpr std::array<std::int8_t, 8> STEP_Y{ 1, 1, 1, 0, 0, -1, -1, -1 };
} // namespace

auto opeu_grid_step_unchecked(GridCell cell, std::uint8_t direction) -> GridCell {
  if (direction >= STEP_X.size()) return cell;
  return { static_cast<std::uint8_t>(cell.x + STEP_X.at(direction)), static_cast<std::uint8_t>(cell.y + STEP_Y.at(direction)) };
}
