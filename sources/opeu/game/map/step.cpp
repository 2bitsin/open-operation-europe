// One step from a cell in one of the eight directions, without a bounds check. Directions 0..7 run from the row below
// (left to right), through the left and right neighbours, to the row above; other directions leave the cell as it is.
// Byte arithmetic wraps.
#include "types.hpp"
#include "map/step.hpp"

#include <array>

namespace {
constexpr std::array<int8_t, 8> STEP_X{ -1, 0, 1, -1, 1, -1, 0, 1 };
constexpr std::array<int8_t, 8> STEP_Y{ 1, 1, 1, 0, 0, -1, -1, -1 };
} // namespace

void opeu_grid_step_unchecked(uint8_t* x, uint8_t* y, uint8_t direction) {
  if (direction < STEP_X.size()) {
    *x = static_cast<uint8_t>(*x + STEP_X[direction]);
    *y = static_cast<uint8_t>(*y + STEP_Y[direction]);
  }
}
