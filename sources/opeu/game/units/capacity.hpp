// The resource capacities of a unit.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include <array>
#include <cstdint>

namespace opeu::game::detail::capacity {

// A unit's ammunition, fuel and food figures.
struct Resources {
  std::uint16_t ammunition;
  std::uint16_t fuel;
  std::uint16_t food;
};

// The three figures in resource order (ammunition, fuel, food), as a unit's or an objective's resources are stored.
[[nodiscard]] constexpr auto Figures(Resources const& resources) noexcept -> std::array<std::uint16_t, 3> {
  return { resources.ammunition, resources.fuel, resources.food };
}

// The DS offsets of the three words that receive a unit's ammunition, fuel and food figures.
struct ResourceWords {
  std::uint16_t ammunition;
  std::uint16_t fuel;
  std::uint16_t food;
};

// What the unit's elements can carry: each element's quantity times its type's weights, summed in wrapping words.
[[nodiscard]] auto UnitCapacities(DataSegment const& data, std::uint16_t unit) -> Resources;

// Stores the unit's capacities into the words at `at`.
auto StoreUnitCapacities(Game& game, std::uint16_t unit, ResourceWords at) -> void;
} // namespace opeu::game::detail::capacity

namespace opeu::game {
using detail::capacity::Figures;
using detail::capacity::ResourceWords;
using detail::capacity::Resources;
using detail::capacity::StoreUnitCapacities;
using detail::capacity::UnitCapacities;
} // namespace opeu::game
