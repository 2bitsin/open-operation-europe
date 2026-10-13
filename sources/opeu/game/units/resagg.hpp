// Resources and capacities of groups, sides and objectives.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "types.hpp"
#include "units/capacity.hpp"

#include <cstdint>

namespace opeu::game::detail::resagg {

// Ammunition, fuel and food summed over many units, in 32 bits.
struct ResourceTotals {
  std::uint32_t ammunition;
  std::uint32_t fuel;
  std::uint32_t food;
};

// The stored resources of the group's ready units.
[[nodiscard]] auto GroupStoredResources(DataSegment const& data, std::uint16_t group) -> ResourceTotals;

// What the group's ready units can carry.
[[nodiscard]] auto GroupCapacities(DataSegment const& data, std::uint16_t group) -> ResourceTotals;

// The stored resources of the side's ready groups.
[[nodiscard]] auto SideStoredResources(DataSegment const& data, std::uint8_t side) -> ResourceTotals;

// What the side's ready groups can carry.
[[nodiscard]] auto SideCapacities(DataSegment const& data, std::uint8_t side) -> ResourceTotals;

// What the objective's stock items can hold, each sum saturating at 60000.
[[nodiscard]] auto ObjectiveCapacities(DataSegment const& data, std::uint16_t objective) -> Resources;
} // namespace opeu::game::detail::resagg

namespace opeu::game {
using detail::resagg::GroupCapacities;
using detail::resagg::GroupStoredResources;
using detail::resagg::ObjectiveCapacities;
using detail::resagg::ResourceTotals;
using detail::resagg::SideCapacities;
using detail::resagg::SideStoredResources;
} // namespace opeu::game

void opeu_objective_capacities(Game& game, uint16_t objective, uint16_t* result_a, uint16_t* result_b, uint16_t* result_c);
