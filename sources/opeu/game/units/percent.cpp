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

auto StoreUnitResourcePercentages(Game& game, std::uint16_t unit, ResourceWords at) -> void {
  StoreResources(game, at, UnitResourcePercentages(Data(game), unit));
}
} // namespace opeu::game::detail::percent

using namespace opeu::game;

void opeu_unit_resource_percentages(Game const& game, uint16_t unit, uint16_t* result_a, uint16_t* result_b, uint16_t* result_c) {
  auto const percentages{ UnitResourcePercentages(Data(game), unit) };
  *result_a = percentages.ammunition;
  *result_b = percentages.fuel;
  *result_c = percentages.food;
}
