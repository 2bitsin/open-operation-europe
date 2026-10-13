// Group and side resource percentages.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "types.hpp"
#include "units/capacity.hpp"

#include <cstdint>

namespace opeu::game::detail::aggpct {

// The group's stored resources against what its ready units can carry.
[[nodiscard]] auto GroupResourcePercentages(DataSegment const& data, std::uint16_t group) -> Resources;

// The side's stored resources against what its ready units can carry, counting units only.
[[nodiscard]] auto SideUnitResourcePercentages(DataSegment const& data, std::uint8_t side) -> Resources;

// The same with the resources of the side's active objectives above their reserves added to what is stored.
[[nodiscard]] auto SideResourcePercentages(Game const& game, std::uint8_t side) -> Resources;
} // namespace opeu::game::detail::aggpct

namespace opeu::game {
using detail::aggpct::GroupResourcePercentages;
using detail::aggpct::SideResourcePercentages;
using detail::aggpct::SideUnitResourcePercentages;
} // namespace opeu::game

void opeu_group_resource_percentages(Game& game, uint16_t group, uint16_t* result_a, uint16_t* result_b, uint16_t* result_c);
void opeu_side_resource_percentages(Game& game, uint8_t side, uint16_t* result_a, uint16_t* result_b, uint16_t* result_c, uint16_t units_only);
