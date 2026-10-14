// The supply orders: a regiment supplies the regiment or the objective next to it. The transfer table is loaded
// from both, the transfer runs (the player's or the computer's) and the amounts go back. Each side of the table holds
// per resource the amount, the capacity and the computer's target.
#include "units/xferunit.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/context.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/recindex.hpp"
#include "map/movetgt.hpp"
#include "types.hpp"
#include "units/aggpct.hpp"
#include "units/capacity.hpp"
#include "units/orders.hpp"
#include "units/resagg.hpp"
#include "units/stats.hpp"
#include "units/transfer-service.hpp"
#include "units/unitname.hpp"
#include "units/value.hpp"
#include "units/xferpnl.hpp"
#include "units/xferstep.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <tuple>

using namespace opeu::game;

namespace {

// The locals the services reach by near address, in each routine's reserve: the two names and the target cell the
// player picks.
constexpr std::uint16_t NAME_BYTES{ 0x20 };

// The computer's targets, in percent of the capacity: low, full, and what it adds to its side's fill.
constexpr std::uint16_t LOW_TARGET{ 10 };
constexpr std::uint16_t FULL_TARGET{ 100 };
constexpr std::uint16_t FILL_MARGIN{ 30 };

using ResourceFigures = std::array<std::uint16_t, RESOURCES>;

// Whether the computer has taken over the regiment's division (it runs that division's transfers).
auto DivisionHeld(Game& game, std::uint16_t unit) -> bool {
  return Record<Group>(Data(game), opeu_unit_group(unit)).flags & Bits(GroupFlag::AI_MARK);
}

auto UnitName(Game& game, std::uint16_t unit, TextTarget buffer) -> TextTarget {
  return opeu_unit_name(game, unit, buffer);
}

// Writes one column of the table's side.
auto SetColumn(DataSegment& data, std::uint8_t side, TransferColumn column, ResourceFigures const& figures) -> void {
  std::ranges::for_each(std::views::zip(data.transfer_table[side], figures), [column](auto&& row) { std::get<0>(row)[column] = std::get<1>(row); });
}

// The amounts of the table's side, in resource order.
[[nodiscard]] auto ColumnOf(DataSegment const& data, std::uint8_t side, TransferColumn column) -> ResourceFigures {
  auto const& rows{ data.transfer_table[side] };
  return { rows[0][column], rows[1][column], rows[2][column] };
}

// Writes the table's amounts back to the regiment.
auto StoreUnit(DataSegment& data, std::uint16_t unit, std::uint8_t side) -> void {
  std::ranges::copy(ColumnOf(data, side, AMOUNT), Record<Unit>(data, unit).resources);
}

// The computer's targets for a regiment, in percent of its capacities: the division's eighth regiment fills to 10%,
// or beside an objective to 100% of a resource the objective holds above its reserve; other regiments fill to their
// side's percentage plus 30, at most 100.
[[nodiscard]] auto Targets(Game const& game, std::uint16_t unit) -> ResourceFigures {
  auto const& data{ Data(game) };
  if (!opeu_category_7(static_cast<std::uint8_t>(opeu_unit_index(unit)))) {
    auto const      fill{ Figures(SideResourcePercentages(game, data.current_side)) };
    ResourceFigures targets{};
    std::ranges::transform(fill, targets.begin(), [](std::uint16_t percent) { return opeu_min16(static_cast<std::uint16_t>(percent + FILL_MARGIN), FULL_TARGET); });
    return targets;
  }
  if (!data.transfer_objective_ptr) return { LOW_TARGET, LOW_TARGET, LOW_TARGET };
  auto const& objective{ Record<Objective>(data, data.transfer_objective_ptr) };
  auto const  target{ [&](std::size_t resource) { return objective.resources[resource] > objective.reserves[resource] ? FULL_TARGET : LOW_TARGET; } };
  return { target(0), target(1), target(2) };
}

// The target regiment or objective of a supply order and the cell it stands on.
struct SupplyTarget {
  std::uint16_t near; // 0 when the target is gone
  std::uint8_t  x;
  std::uint8_t  y;
};

// The supply order next to the current regiment: once beside the target and the order's hours are spent, `transfer`
// moves the supplies; a lost target or a done transfer ends the order.
auto Supply(Game& game, SupplyTarget const& target, auto transfer) -> std::uint16_t {
  auto& data{ Data(game) };
  if (target.near) {
    if (!opeu_move_adjacent(game, target.x, target.y)) return 0;
    auto const hours{ opeu_transfer_level(game) };
    // The order takes its hours before the supplies move.
    if (!opeu_move_level_lower(game, Near(data, Record<Unit>(data, data.current_unit_ptr).orders[0]), hours)) return 1;
    transfer(game, data.current_unit_ptr, target.near);
  }
  opeu_restore_unit_order(game, data.current_unit_ptr);
  return 1;
}

// The cell bytes a target service wrote, with the target it found.
[[nodiscard]] auto TargetOf(OrderTarget const& target) -> SupplyTarget {
  return { target.near, target.cell.x, target.cell.y };
}
} // namespace

// Loads a regiment into the table's `side`: its amounts, its capacities and the computer's targets in percent of
// each capacity.
void opeu_transfer_load_unit(Game& game, uint16_t unit, uint8_t side) {
  auto&       data{ Data(game) };
  auto const& regiment{ Record<Unit>(data, unit) };
  SetColumn(data, side, AMOUNT, { regiment.resources[0], regiment.resources[1], regiment.resources[2] });
  auto const capacities{ Figures(UnitCapacities(data, unit)) };
  SetColumn(data, side, CAPACITY, capacities);
  auto const      targets{ Targets(game, unit) };
  ResourceFigures percents{};
  std::ranges::transform(capacities, targets, percents.begin(), opeu_percent);
  SetColumn(data, side, TARGET, percents);
}

// Loads an objective into the table's right side: its capacities, its reserves as the targets and its amounts.
void opeu_transfer_load_objective(Game& game, uint16_t objective) {
  auto&       data{ Data(game) };
  auto const& city{ Record<Objective>(data, objective) };
  SetColumn(data, 1, CAPACITY, Figures(ObjectiveCapacities(data, objective)));
  SetColumn(data, 1, TARGET, { city.reserves[0], city.reserves[1], city.reserves[2] });
  SetColumn(data, 1, AMOUNT, { city.resources[0], city.resources[1], city.resources[2] });
}

// The transfer between two regiments, repeated until it is confirmed (or the computer finishes); the player runs it
// when a player commands the side, unless the computer has taken over both divisions.
void opeu_transfer_units(Game& game, uint16_t first, uint16_t second) {
  ServiceLocals const names{ 2 * NAME_BYTES };
  auto&               data{ Data(game) };
  data.transfer_objective_ptr = 0;
  do {
    opeu_transfer_load_unit(game, first, 0);
    opeu_transfer_load_unit(game, second, 1);
    // The second name is taken first (the original's right-to-left arguments).
    auto const second_name{ UnitName(game, second, names.At(NAME_BYTES)) };
    auto const first_name{ UnitName(game, first, names.At(0)) };
    opeu_transfer_draw(game, first_name, second_name);
  } while (!opeu_transfer_run(game, data.side_control[data.current_side] == SIDE_PLAYED && (!DivisionHeld(game, first) || !DivisionHeld(game, second))));
  StoreUnit(data, first, 0);
  StoreUnit(data, second, 1);
}

// The transfer between a regiment and an objective.
void opeu_transfer_objective(Game& game, uint16_t unit, uint16_t objective) {
  ServiceLocals const name{ NAME_BYTES };
  auto&               data{ Data(game) };
  data.transfer_objective_ptr = objective;
  do {
    opeu_transfer_load_unit(game, unit, 0);
    opeu_transfer_load_objective(game, objective);
    opeu_transfer_draw(game, UnitName(game, unit, name.At(0)), objective);
  } while (!opeu_transfer_run(game, data.side_control[data.current_side] == SIDE_PLAYED && !DivisionHeld(game, unit)));
  StoreUnit(data, unit, 0);
  std::ranges::copy(ColumnOf(data, 1, AMOUNT), Record<Objective>(data, objective).resources);
  opeu_daily_context(game, static_cast<uint8_t>(data.current_side));
}

// The hours a supply order of the current regiment takes: 4, plus its experience / 24, less its training / 32.
uint8_t opeu_transfer_level(Game& game) {
  auto const unit{ Data(game).current_unit_ptr };
  auto const training{ static_cast<std::uint16_t>(opeu_unit_average10(game, unit) >> 5) };
  return static_cast<std::uint8_t>(opeu_unit_average11(game, unit) / 24 - training + 4);
}

uint16_t opeu_order_supply_objective(Game& game) {
  return Supply(game, TargetOf(opeu_move_objective_target(game, Data(game).current_side)), opeu_transfer_objective);
}

uint16_t opeu_order_supply_unit(Game& game) {
  return Supply(game, TargetOf(opeu_move_unit_target(game)), opeu_transfer_units);
}
