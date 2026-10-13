// The resource capacities of a unit.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include <cstdint>

namespace opeu::game::detail::capacity {

// A unit's ammunition, fuel and food figures.
struct Resources {
  std::uint16_t ammunition;
  std::uint16_t fuel;
  std::uint16_t food;
};

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

// Stores the three figures into the words at `at`.
auto StoreResources(Game& game, ResourceWords at, Resources resources) -> void;
} // namespace opeu::game::detail::capacity

namespace opeu::game {
using detail::capacity::ResourceWords;
using detail::capacity::Resources;
using detail::capacity::StoreResources;
using detail::capacity::StoreUnitCapacities;
using detail::capacity::UnitCapacities;
} // namespace opeu::game

void opeu_unit_capacities(Game const& game, uint16_t unit, uint16_t* result_a, uint16_t* result_b, uint16_t* result_c);
