// An objective's resources as percentages of its storage capacity.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "types.hpp"
#include "units/capacity.hpp"

#include <cstdint>

namespace opeu::game::detail::objpct {

// The objective's ammunition, fuel and food as percentages of what its stock can hold.
[[nodiscard]] auto ObjectiveResourcePercentages(DataSegment const& data, std::uint16_t objective) -> Resources;
} // namespace opeu::game::detail::objpct

namespace opeu::game {
using detail::objpct::ObjectiveResourcePercentages;
} // namespace opeu::game
