// An objective's ammunition, fuel and food as percentages of its storage capacity.
#include "units/objpct.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "types.hpp"
#include "units/ratio.hpp"
#include "units/resagg.hpp"

namespace opeu::game::detail::objpct {

auto ObjectiveResourcePercentages(DataSegment const& data, std::uint16_t objective) -> Resources {
  auto const  capacities{ ObjectiveCapacities(data, objective) };
  auto const& record{ Record<Objective>(data, objective) };
  return {
    opeu_resource_percent(record.resources[0], capacities.ammunition),
    opeu_resource_percent(record.resources[1], capacities.fuel),
    opeu_resource_percent(record.resources[2], capacities.food),
  };
}
} // namespace opeu::game::detail::objpct

using namespace opeu::game;

void opeu_objective_resource_percentages(Game& game, uint16_t objective, uint16_t* result_a, uint16_t* result_b, uint16_t* result_c) {
  auto const percentages{ ObjectiveResourcePercentages(Data(game), objective) };
  *result_a = percentages.ammunition;
  *result_b = percentages.fuel;
  *result_c = percentages.food;
}
