// A unit's ammunition, fuel and food as percentages of what its elements can carry.
#include "units/percent.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "types.hpp"
#include "units/capacity.hpp"
#include "units/ratio.hpp"

namespace opeu::game::detail::percent {

auto UnitResourcePercentages(DataSegment const& data, std::uint16_t unit) -> Resources {
  auto const  capacities{ UnitCapacities(data, unit) };
  auto const& record{ Record<Unit>(data, unit) };
  return {
    opeu_resource_percent(record.resources[0], capacities.ammunition),
    opeu_resource_percent(record.resources[1], capacities.fuel),
    opeu_resource_percent(record.resources[2], capacities.food),
  };
}
} // namespace opeu::game::detail::percent
