// Group and side resource percentages: stored resources against capacity. For a side, unless only units count, the
// resources of its active objectives above their reserves add to the stored amount; capacities still come only from
// active units and groups.
#include "units/aggpct.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "core/limits.hpp"
#include "map/query.hpp"
#include "types.hpp"
#include "units/ratio.hpp"
#include "units/resagg.hpp"

using namespace opeu::game;

namespace {
[[nodiscard]] auto Percent(std::uint32_t stored, std::uint32_t capacity) -> std::uint16_t {
  return opeu_resource_percent_wide(static_cast<std::int32_t>(stored), static_cast<std::int32_t>(capacity));
}
} // namespace

void opeu_group_resource_percentages(Game& game, uint16_t group, uint16_t* result_a, uint16_t* result_b, uint16_t* result_c) {
  std::uint32_t stored_a, stored_b, stored_c, capacity_a, capacity_b, capacity_c;
  opeu_group_resources(game, group, &stored_a, &stored_b, &stored_c, 1);
  opeu_group_resources(game, group, &capacity_a, &capacity_b, &capacity_c, 0);
  *result_a = Percent(stored_a, capacity_a);
  *result_b = Percent(stored_b, capacity_b);
  *result_c = Percent(stored_c, capacity_c);
}

void opeu_side_resource_percentages(Game& game, uint8_t side, uint16_t* result_a, uint16_t* result_b, uint16_t* result_c, uint16_t units_only) {
  auto&         data{ Data(game) };
  std::uint32_t stored[3], capacity[3];
  opeu_side_resources(game, side, &stored[0], &stored[1], &stored[2], 1);
  opeu_side_resources(game, side, &capacity[0], &capacity[1], &capacity[2], 0);
  if (!units_only) {
    for (auto const& objective : data.objectives)
      if (objective.active && opeu_objective_side(game, Near(data, objective)) == side)
        for (auto resource{ 0 }; resource < 3; ++resource)
          stored[resource] += static_cast<std::uint16_t>(objective.resources[resource] - opeu_min16(objective.resources[resource], objective.reserves[resource]));
  }
  *result_a = Percent(stored[0], capacity[0]);
  *result_b = Percent(stored[1], capacity[1]);
  *result_c = Percent(stored[2], capacity[2]);
}
