// An objective's ammunition, fuel and food as percentages of its storage capacity.
#include "units/objpct.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "types.hpp"
#include "units/ratio.hpp"
#include "units/resagg.hpp"

using namespace opeu::game;

void opeu_objective_resource_percentages(Game& game, uint16_t objective, uint16_t* result_a, uint16_t* result_b, uint16_t* result_c) {
  uint16_t capacity_c, capacity_b, capacity_a;
  opeu_objective_capacities(game, objective, &capacity_a, &capacity_b, &capacity_c);
  auto const& record{ Record<Objective>(Data(game), objective) };
  *result_a = opeu_resource_percent(record.resources[0], capacity_a);
  *result_b = opeu_resource_percent(record.resources[1], capacity_b);
  *result_c = opeu_resource_percent(record.resources[2], capacity_c);
}
