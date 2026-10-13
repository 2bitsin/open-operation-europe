// Stored resources or capacities summed over a group's or a side's ready units, and the capacities of an objective's
// stock.
#include "units/resagg.hpp"
#include "common/data-segment.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "core/recindex.hpp"
#include "types.hpp"
#include "units/capacity.hpp"

#include <algorithm>
#include <ranges>
#include <span>

namespace opeu::game::detail::resagg {
namespace {
// An objective's stock saturates each sum at this amount.
constexpr std::uint32_t STOCK_LIMIT{ 60000 };

// Adds three figures (a unit's or a group's) to the totals.
constexpr auto ADD{ [](ResourceTotals const& totals, auto const& figures) -> ResourceTotals {
  return { totals.ammunition + figures.ammunition, totals.fuel + figures.fuel, totals.food + figures.food };
} };

[[nodiscard]] auto IsReady(Unit const& unit) -> bool {
  return (unit.flags & Bits(UnitFlag::READY)) != 0;
}

[[nodiscard]] auto IsReady(Group const& group) -> bool {
  return (group.flags & Bits(GroupFlag::READY)) != 0;
}

[[nodiscard]] auto StoredResources(Unit const& unit) -> Resources {
  return { unit.resources[0], unit.resources[1], unit.resources[2] };
}

// Sums `figures` of each ready unit of the group.
[[nodiscard]] auto SumGroup(DataSegment const& data, std::uint16_t group, auto figures) -> ResourceTotals {
  auto const units{ GroupUnits(data, opeu_group_side(group), opeu_group_index(group)) };
  return std::ranges::fold_left(units | std::views::filter([](Unit const& unit) { return IsReady(unit); }) | std::views::transform(figures), ResourceTotals{}, ADD);
}

// Sums `group_totals` of each ready group of the side.
[[nodiscard]] auto SumSide(DataSegment const& data, std::uint8_t side, auto group_totals) -> ResourceTotals {
  auto const groups{ std::span{ data.groups }.subspan(GROUPS_PER_SIDE * side, GROUPS_PER_SIDE) };
  auto const totals{ [&](Group const& group) { return group_totals(data, Near(data, group)); } };
  return std::ranges::fold_left(groups | std::views::filter([](Group const& group) { return IsReady(group); }) | std::views::transform(totals), ResourceTotals{}, ADD);
}

// Adds `amount` to `sum` without wrapping, capped at the stock limit.
[[nodiscard]] auto StockSum(std::uint16_t sum, std::uint16_t amount) -> std::uint16_t {
  return static_cast<std::uint16_t>(std::min(std::uint32_t{ sum } + amount, STOCK_LIMIT));
}
} // namespace

auto GroupStoredResources(DataSegment const& data, std::uint16_t group) -> ResourceTotals {
  return SumGroup(data, group, [](Unit const& unit) { return StoredResources(unit); });
}

auto GroupCapacities(DataSegment const& data, std::uint16_t group) -> ResourceTotals {
  return SumGroup(data, group, [&](Unit const& unit) { return UnitCapacities(data, Near(data, unit)); });
}

auto SideStoredResources(DataSegment const& data, std::uint8_t side) -> ResourceTotals {
  return SumSide(data, side, GroupStoredResources);
}

auto SideCapacities(DataSegment const& data, std::uint8_t side) -> ResourceTotals {
  return SumSide(data, side, GroupCapacities);
}

auto ObjectiveCapacities(DataSegment const& data, std::uint16_t objective) -> Resources {
  Resources capacities{ 0, 0, 0 };
  for (auto const& item : Record<Objective>(data, objective).items | std::views::filter([](Item const& item) { return item.quantity != 0; })) {
    auto const& type{ data.element_types[item.type] };
    auto const  category{ type.category };
    auto const  quantity{ item.quantity };
    capacities.ammunition = StockSum(capacities.ammunition, static_cast<std::uint16_t>(unsigned{ data.ammunition_capacities[category] } * quantity));
    capacities.fuel       = StockSum(capacities.fuel, static_cast<std::uint16_t>(unsigned{ type.fuel_capacity } * quantity));
    capacities.food       = StockSum(capacities.food, static_cast<std::uint16_t>(unsigned{ data.food_capacities[category] } * quantity));
  }
  return capacities;
}
} // namespace opeu::game::detail::resagg

using namespace opeu::game;

void opeu_objective_capacities(Game& game, uint16_t objective, uint16_t* result_a, uint16_t* result_b, uint16_t* result_c) {
  auto const capacities{ ObjectiveCapacities(Data(game), objective) };
  *result_a = capacities.ammunition;
  *result_b = capacities.fuel;
  *result_c = capacities.food;
}
