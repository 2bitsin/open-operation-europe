// A unit's resources as percentages of its carrying capacity.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "types.hpp"
#include "units/capacity.hpp"

#include <cstdint>

namespace opeu::game::detail::percent {

// The unit's ammunition, fuel and food as percentages of what its elements can carry.
[[nodiscard]] auto UnitResourcePercentages(DataSegment const& data, std::uint16_t unit) -> Resources;

// Stores the unit's resource percentages into the words at `at`.
auto StoreUnitResourcePercentages(Game& game, std::uint16_t unit, ResourceWords at) -> void;
} // namespace opeu::game::detail::percent

namespace opeu::game {
using detail::percent::StoreUnitResourcePercentages;
using detail::percent::UnitResourcePercentages;
} // namespace opeu::game

void opeu_unit_resource_percentages(Game const& game, uint16_t unit, uint16_t* result_a, uint16_t* result_b, uint16_t* result_c);
