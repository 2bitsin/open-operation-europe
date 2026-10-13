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

#include <algorithm>
#include <ranges>

namespace opeu::game::detail::aggpct {
namespace {
[[nodiscard]] auto Percent(std::uint32_t stored, std::uint32_t capacity) -> std::uint16_t {
  return opeu_resource_percent_wide(static_cast<std::int32_t>(stored), static_cast<std::int32_t>(capacity));
}

[[nodiscard]] auto Percentages(ResourceTotals const& stored, ResourceTotals const& capacities) -> Resources {
  return { Percent(stored.ammunition, capacities.ammunition), Percent(stored.fuel, capacities.fuel), Percent(stored.food, capacities.food) };
}

// What an objective holds above its reserve of one resource, in a wrapping word.
[[nodiscard]] auto Surplus(Objective const& objective, std::size_t resource) -> std::uint16_t {
  return static_cast<std::uint16_t>(objective.resources[resource] - opeu_min16(objective.resources[resource], objective.reserves[resource]));
}

// The side's stored resources plus the surplus of each of its active objectives.
[[nodiscard]] auto StoredWithSurplus(Game const& game, std::uint8_t side) -> ResourceTotals {
  auto const& data{ Data(game) };
  auto const  owned{ [&](Objective const& objective) { return objective.active && opeu_objective_side(game, Near(data, objective)) == side; } };
  auto const  add{ [](ResourceTotals const& totals, Objective const& objective) -> ResourceTotals {
    return { totals.ammunition + Surplus(objective, 0), totals.fuel + Surplus(objective, 1), totals.food + Surplus(objective, 2) };
  } };
  return std::ranges::fold_left(data.objectives | std::views::filter(owned), SideStoredResources(data, side), add);
}
} // namespace

auto GroupResourcePercentages(DataSegment const& data, std::uint16_t group) -> Resources {
  return Percentages(GroupStoredResources(data, group), GroupCapacities(data, group));
}

auto SideUnitResourcePercentages(DataSegment const& data, std::uint8_t side) -> Resources {
  return Percentages(SideStoredResources(data, side), SideCapacities(data, side));
}

auto SideResourcePercentages(Game const& game, std::uint8_t side) -> Resources {
  return Percentages(StoredWithSurplus(game, side), SideCapacities(Data(game), side));
}
} // namespace opeu::game::detail::aggpct
