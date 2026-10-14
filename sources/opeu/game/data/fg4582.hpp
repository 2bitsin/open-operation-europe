// The path search's work areas, initially zero: a queue of cell numbers (cell = row * 40 + column) and a 40 x 40
// word grid of path costs that is first filled with ff bytes.
#pragma once

#include <array>
#include <cstdint>

namespace opeu::game::detail::fg4582 {

inline constexpr std::array<std::uint16_t, 1600> PATH_CELL_QUEUE{};

inline constexpr std::array<std::array<std::uint16_t, 40>, 40> PATH_COST_GRID{};

} // namespace opeu::game::detail::fg4582

namespace opeu::game {
using detail::fg4582::PATH_CELL_QUEUE;
using detail::fg4582::PATH_COST_GRID;
} // namespace opeu::game
