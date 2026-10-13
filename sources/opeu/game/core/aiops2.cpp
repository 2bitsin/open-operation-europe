// The computer side's supply planning: the neediest objective or unit and the airlift that supplies it, and the
// daytime planning step that also plans strikes and drops.
#include "core/aiops2.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "core/aibudget.hpp"
#include "core/aiops1.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/random.hpp"
#include "map/query.hpp"
#include "map/support.hpp"
#include "units/capacity.hpp"
#include "units/orders.hpp"
#include "units/resagg.hpp"

using namespace opeu::game;

namespace {

constexpr std::uint16_t NO_NEED{ 999 };      // above any percentage
constexpr std::uint16_t NEEDY_PERCENT{ 20 }; // supplies below this share of capacity call for an airlift
constexpr std::uint16_t OBJECTIVE_FULL{ 60000 };
constexpr std::uint8_t  FIRST_HOUR{ 8 };
constexpr std::uint8_t  LAST_HOUR{ 16 };

// The lowest of the three supply percentages (stock against capacity) below `lowest`, or `lowest`.
auto LowestPercent(std::uint16_t const (&stock)[3], std::uint16_t const (&capacity)[3], std::uint16_t lowest, bool& lower) -> std::uint16_t {
  lower = false;
  for (int i{ 0 }; i < 3; ++i) {
    auto const percent{ opeu_mul_div(stock[i], 100, capacity[i]) };
    if (percent < lowest) {
      lowest = percent;
      lower  = true;
    }
  }
  return lowest;
}
} // namespace

// The side's eligible objective with the lowest supply percentage, if that is below 20; otherwise 0.
uint16_t opeu_ai_neediest_objective(uint8_t ds[65536]) {
  auto&         data{ Data(ds) };
  std::uint16_t neediest{ 0 };
  std::uint16_t lowest{ NO_NEED };
  for (std::uint8_t i{ 0 }; i < OBJECTIVE_LINKS_PER_SIDE; ++i) {
    auto const near{ data.objective_links[data.current_side * OBJECTIVE_LINKS_PER_SIDE + i] };
    if (!near || !Record<Objective>(data, near).active || !AiObjectiveEligible(ds, near) || opeu_objective_side(ds, near) != data.current_side) continue;
    auto const&   objective{ Record<Objective>(data, near) };
    std::uint16_t stock[3]{ objective.resources[0], objective.resources[1], objective.resources[2] };
    std::uint16_t capacity[3];
    opeu_objective_capacities(ds, near, &capacity[0], &capacity[1], &capacity[2]);
    bool lower{};
    lowest = LowestPercent(stock, capacity, lowest, lower);
    if (lower) neediest = near;
  }
  return lowest < NEEDY_PERCENT ? neediest : 0;
}

// The same search over the side's 32 ready units that can act.
uint16_t opeu_ai_neediest_unit(uint8_t ds[65536]) {
  auto&         data{ Data(ds) };
  std::uint16_t neediest{ 0 };
  std::uint16_t lowest{ NO_NEED };
  for (unsigned i{ 0 }; i < UNITS_PER_SIDE; ++i) {
    auto&      unit{ data.units[data.current_side * UNITS_PER_SIDE + i] };
    auto const near{ Near(data, unit) };
    if (!(unit.flags & Bits(UnitFlag::READY)) || !opeu_unit_selection_eligible(ds, near)) continue;
    std::uint16_t stock[3]{ unit.resources[0], unit.resources[1], unit.resources[2] };
    std::uint16_t capacity[3];
    opeu_unit_capacities(ds, near, &capacity[0], &capacity[1], &capacity[2]);
    bool lower{};
    lowest = LowestPercent(stock, capacity, lowest, lower);
    if (lower) neediest = near;
  }
  return lowest < NEEDY_PERCENT ? neediest : 0;
}

// Twice the amount, the amount capped at 30000.
uint16_t opeu_ai_079c(uint16_t amount) {
  return static_cast<uint16_t>(opeu_min16(amount, 30000) * 2);
}

// One time in twelve, plans a supply airlift to the neediest objective (or else unit) within the side's spare
// transports and returns 1 (0 when nothing needs or can be lifted). Otherwise returns the nonzero random draw.
uint16_t opeu_ai_plan_supply(uint8_t ds[65536]) {
  auto const draw{ opeu_rand_bounded(12) };
  if (draw) return draw;
  auto& data{ Data(ds) };
  auto& lift{ Record<Side>(data, data.current_side_ptr).lift };
  lift.planes = 0;
  auto const planes{ opeu_ai_spare_cargo(ds) };
  if (!planes) return 0;
  std::uint16_t budget{ planes };
  if (auto const objective_near{ opeu_ai_neediest_objective(ds) }) {
    auto const& objective{ Record<Objective>(data, objective_near) };
    lift.x = objective.x;
    lift.y = objective.y;
    for (std::uint8_t i{ 0 }; i < 3; ++i) lift.supplies[i] = AiSupplyShare(ds, &budget, opeu_ai_079c(static_cast<std::uint16_t>(OBJECTIVE_FULL - objective.resources[i])), i);
  } else if (auto const unit_near{ opeu_ai_neediest_unit(ds) }) {
    auto const&   unit{ Record<Unit>(data, unit_near) };
    std::uint16_t capacity[3];
    opeu_unit_capacities(ds, unit_near, &capacity[0], &capacity[1], &capacity[2]);
    lift.x = unit.x;
    lift.y = unit.y;
    for (std::uint8_t i{ 0 }; i < 3; ++i) lift.supplies[i] = AiSupplyShare(ds, &budget, opeu_ai_079c(static_cast<std::uint16_t>(capacity[i] - unit.resources[i])), i);
    opeu_set_unit_order(ds, unit_near, 0, unit.x, 0, 0xff, unit.y);
  } else
    return 0;
  lift.planes = static_cast<std::uint16_t>(planes - opeu_min16(budget, planes));
  lift.hours  = 1;
  lift.turns  = 7;
  return 1;
}

// In daytime (8 to 16 h): plans a strike, and a drop or an airlift when the side's day allows them. 1 in daytime.
uint16_t opeu_ai_daytime_plans(uint8_t ds[65536]) {
  auto const& data{ Data(ds) };
  if (data.hour < FIRST_HOUR || data.hour > LAST_HOUR) return 0;
  opeu_ai_002a(ds);
  auto const flags{ data.side_flags[data.current_side] };
  if (flags & Bits(SideFlag::AIR_DROP_READY)) opeu_ai_03e2(ds);
  if (flags & Bits(SideFlag::AIR_CARGO_READY)) opeu_ai_plan_supply(ds);
  return 1;
}
