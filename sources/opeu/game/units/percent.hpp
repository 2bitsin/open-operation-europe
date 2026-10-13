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
} // namespace opeu::game::detail::percent

namespace opeu::game {
using detail::percent::UnitResourcePercentages;
} // namespace opeu::game
