// The computer side's orders for its current unit: engineers route around mines and rivers and mine near the
// objectives their division holds, supply units carry supplies between cities and units, and combat units take or hold
// their division's objectives, resupply and attack enemy units within reach. Paths come from a flood fill of step
// costs over the map.
#include "core/unitai.hpp"
#include "common/grid-cell.hpp"
#include "core/aiterr.hpp"
#include "core/grid-neighbours.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/recindex.hpp"
#include "core/selunit.hpp"
#include "core/sideutil.hpp"
#include "map/cellflag.hpp"
#include "map/distance.hpp"
#include "map/grid.hpp"
#include "map/objeleg.hpp"
#include "map/query.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/aggpct.hpp"
#include "units/capacity.hpp"
#include "units/orders.hpp"
#include "units/stats.hpp"
#include "units/value.hpp"

#include "common/data-segment.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "common/map-grid.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <vector>

using namespace opeu::game;

namespace {

constexpr std::uint8_t  UNREACHED{ 0xff };      // a distance or step cost no cell has
constexpr std::uint16_t UNVISITED{ 0xffff };    // the path cost of a cell the fill has not reached
constexpr std::uint8_t  IMPASSABLE_STEP{ 255 }; // the step cost of a cell the unit cannot enter
constexpr std::uint8_t  ENGINEER_SLOT{ 6 };     // the engineer unit of a division
constexpr std::uint8_t  SUPPLY_SLOT{ 7 };       // the supply unit of a division

// The first six unit slots of a division are its combat units.
constexpr std::size_t COMBAT_SLOTS{ 6 };

constexpr auto IsCombatSlot(std::uint8_t slot) -> bool {
  return slot < COMBAT_SLOTS;
}

auto Distance1D(int difference) -> int {
  return static_cast<int16_t>(opeu_abs16_bits(difference));
}

// The near pointer of a record, 0 for none.
template <typename T>
auto NearOrNull(DataSegment const& data, T const* record) -> std::uint16_t {
  return record ? Near(data, *record) : 0;
}

auto UnitAt(DataSegment& data, std::uint16_t near) -> Unit* {
  return near ? &Record<Unit>(data, near) : nullptr;
}

auto ObjectiveAt(Game& game, std::uint8_t x, std::uint8_t y) -> Objective* {
  auto const near{ opeu_find_objective_at(game, x, y) };
  return near ? &Record<Objective>(Data(game), near) : nullptr;
}

// Gives the current unit an order at a cell.
void IssueOrder(Game& game, UnitOrder kind, GridCell cell) {
  opeu_set_unit_order(game, Data(game).current_unit_ptr, { .target = 0, .cell = cell, .save_current = false, .code = Bits(kind) });
}

// Gives the current unit an order against a target record (none: the order's cell is 0, 0).
void IssueOrderAgainst(Game& game, UnitOrder kind, std::uint16_t target) {
  opeu_set_unit_order(game, Data(game).current_unit_ptr,
                      {
                          .target = target, .cell = { 0, 0 },
                               .save_current = false, .code = Bits(kind)
  });
}

// The path cost of every map cell from a flood fill outward from a start cell, each step costing the unit's terrain
// rating of the cell entered, and the walk back down those costs.
class PathCosts {
public:
  // Fills the costs in order of cost from move.from, stopping as soon as a step reaches move.to.
  void Fill(Game& game, CellMove move, Unit const& unit) {
    auto const& data{ Data(game) };
    _cost.fill(UNVISITED);
    _queue.clear();
    _queue.push_back(Index(move.from));
    _cost[_queue.front()] = 0;
    for (std::size_t head{ 0 }; head != _queue.size(); ++head) {
      auto const cell{ CellOf(_queue[head]) };
      auto const cost{ _cost[_queue[head]] };
      for (auto const step : CellsAround(cell)) {
        if (!opeu_map_code(game, step.x, step.y)) continue;
        if (step == move.to) return;
        auto& entered{ _cost[Index(step)] };
        if (entered != UNVISITED) continue;
        auto const terrain{ opeu_best_terrain(game, step.x, step.y, Near(data, unit)) };
        if (terrain == IMPASSABLE_STEP) continue;
        entered = static_cast<std::uint16_t>(terrain + cost);
        Enqueue(Index(step));
      }
    }
  }

  // Walks from move.from down the costs to move.to, each step to the cheapest neighbour (on a tie, one no farther from
  // the target than the start was); the cell the walk stops on when it is one the side has marked or a river.
  [[nodiscard]] auto Descend(Game& game, CellMove move) const -> std::optional<GridCell> {
    auto const& data{ Data(game) };
    auto const  start_dx{ Distance1D(move.from.x - move.to.x) };
    auto const  start_dy{ Distance1D(move.from.y - move.to.y) };
    for (auto cell{ move.from }; cell != move.to;) {
      std::uint16_t best{ UNVISITED };
      auto          next{ cell };
      for (auto const step : CellsAround(cell)) {
        auto const cost{ _cost[Index(step)] };
        if (cost < best || (cost == best && Distance1D(step.x - move.to.x) <= start_dx && Distance1D(step.y - move.to.y) <= start_dy)) {
          next = step;
          best = cost;
        }
      }
      cell = next;
      if (opeu_cell_marked(game, { cell.x, cell.y }, data.current_side) || opeu_map_code(game, cell.x, cell.y) == Bits(Terrain::RIVER)) return cell;
    }
    return std::nullopt;
  }

private:
  [[nodiscard]] static auto Index(GridCell cell) -> std::uint16_t {
    return static_cast<std::uint16_t>(cell.x + cell.y * MAP_WIDTH);
  }

  [[nodiscard]] static auto CellOf(std::uint16_t index) -> GridCell {
    return { static_cast<std::uint8_t>(index % MAP_WIDTH), static_cast<std::uint8_t>(index / MAP_WIDTH) };
  }

  // Queues the cell after every queued cell of no greater cost: cells of equal cost stay in arrival order.
  void Enqueue(std::uint16_t cell) {
    auto const cost_of{ [this](std::uint16_t queued) { return _cost[queued]; } };
    _queue.insert(std::ranges::upper_bound(_queue, _cost[cell], std::ranges::less{}, cost_of), cell);
  }

  std::array<std::uint16_t, MAP_WIDTH * MAP_HEIGHT> _cost{};
  std::vector<std::uint16_t>                        _queue;
};

// Routes the unit towards its order's destination: when the path down the cost grid runs into a cell marked by the
// side, the unit clears the mines there; into a river, it bridges it.
auto PathOrderStep(Game& game, Unit& unit) -> bool {
  auto& data{ Data(game) };
  if (!(unit.flags & Bits(UnitFlag::READY))) return false;
  auto const destination{ UnitOrderDestination(game, Near(data, unit)) };
  if (!destination.valid || !destination.cell) return false;
  GridCell const goal{ destination.cell->x, destination.cell->y };
  GridCell const start{ unit.x, unit.y };
  PathCosts      costs;
  costs.Fill(game, { goal, start }, unit);
  auto const stop{ costs.Descend(game, { start, goal }) };
  if (!stop) return false;
  auto const [x, y]{ *stop };
  if (opeu_cell_marked(game, { x, y }, data.current_side))
    IssueOrder(game, UnitOrder::CLEAR_MINES, { x, y });
  else if (opeu_map_code(game, x, y) == Bits(Terrain::RIVER))
    IssueOrder(game, UnitOrder::BUILD_BRIDGE, { x, y });
  return true;
}

// Whether the objective holds more (Overstocked) or less (Understocked) of any supply than its reserve.
auto Overstocked(Objective const& objective) -> bool {
  return objective.resources[0] > objective.reserves[0] || objective.resources[1] > objective.reserves[1] || objective.resources[2] > objective.reserves[2];
}

auto Understocked(Objective const& objective) -> bool {
  return objective.resources[0] < objective.reserves[0] || objective.resources[1] < objective.reserves[1] || objective.resources[2] < objective.reserves[2];
}

// The side's eligible objective nearest the current unit that satisfies the predicate.
template <typename Predicate>
auto NearestObjective(Game& game, Predicate predicate) -> Objective* {
  auto&        data{ Data(game) };
  auto const&  current{ CurrentUnit(data) };
  Objective*   nearest{ nullptr };
  std::uint8_t best{ UNREACHED };
  for (auto& objective : data.objectives)
    if (objective.active && opeu_objective_side(game, Near(data, objective)) == data.current_side && opeu_objective_eligible(game, Near(data, objective))) {
      auto const distance{ GridDistance({ current.x, current.y }, { objective.x, objective.y }) };
      if (predicate(objective) && distance < best) {
        nearest = &objective;
        best    = distance;
      }
    }
  return nearest;
}

// Whether any of the unit's supplies is below its percentage (floor) of the unit's capacity.
auto BelowPercent(Game& game, Unit const& unit, Resources const& floor) -> bool {
  auto const capacity{ UnitCapacities(Data(game), Near(Data(game), unit)) };
  return opeu_mul_div(unit.resources[0], 100, capacity.ammunition) < floor.ammunition || opeu_mul_div(unit.resources[1], 100, capacity.fuel) < floor.fuel
         || opeu_mul_div(unit.resources[2], 100, capacity.food) < floor.food;
}

// The supported unit of the current division nearest the current unit with a supply below the given percentages.
auto NearestUnitInNeed(Game& game, Resources const& floor) -> Unit* {
  auto&        data{ Data(game) };
  auto const&  current{ CurrentUnit(data) };
  Unit*        nearest{ nullptr };
  std::uint8_t best{ UNREACHED };
  for (auto& unit : GroupUnits(data, data.current_side, data.current_group))
    if ((unit.flags & Bits(UnitFlag::READY)) && &unit != &current && opeu_unit_support_condition(game, Near(data, unit))) {
      auto const distance{ GridDistance({ current.x, current.y }, { unit.x, unit.y }) };
      if (BelowPercent(game, unit, floor) && distance < best) {
        nearest = &unit;
        best    = distance;
      }
    }
  return nearest;
}

// Whether the objective has supplies to take: any at all when it is the enemy's, any above its reserve when it is
// the side's own.
auto HasRemaining(Game& game, Objective const& objective) -> bool {
  auto const& data{ Data(game) };
  auto        ammo{ objective.resources[0] };
  auto        fuel{ objective.resources[1] };
  auto        food{ objective.resources[2] };
  if (opeu_objective_side(game, Near(data, objective)) == data.current_side) {
    ammo = SubWord(ammo, objective.reserves[0]);
    fuel = SubWord(fuel, objective.reserves[1]);
    food = SubWord(food, objective.reserves[2]);
  }
  return ammo || fuel || food;
}

// Whether one of the current division's tasks lies at the objective.
auto GroupTargets(Game& game, Objective const* objective) -> bool {
  auto const& data{ Data(game) };
  for (auto const& task : GroupOf(data, data.current_side, data.current_group).tasks)
    if (task.kind != Bits(TaskKind::NONE)) {
      auto const* at{ ObjectiveAt(game, task.x, task.y) };
      if (at && at == objective) return true;
    }
  return false;
}

// The combat units an objective needs: one, plus one for every two levels.
auto RequiredUnits(Objective const& objective) -> std::uint8_t {
  return static_cast<std::uint8_t>(((objective.flags & Bits(ObjectiveFlag::LEVEL_MASK)) >> 1) + 1);
}

// Whether enough combat units of the current division already have an order at the objective.
auto FullyAssigned(Game& game, Objective const* objective) -> bool {
  auto&      data{ Data(game) };
  auto const assigned{ std::ranges::count_if(GroupUnits(data, data.current_side, data.current_group).first<COMBAT_SLOTS>(), [&](Unit const& unit) {
    auto const& order{ unit.orders[0] };
    return (unit.flags & Bits(UnitFlag::READY)) && order.kind != Bits(UnitOrder::NONE) && ObjectiveAt(game, order.cell.x, order.cell.y) == objective;
  }) };
  return RequiredUnits(*objective) <= assigned;
}

// Whether the enemy unit is worth attacking: a special unit, or any unit while the side controls nothing and the
// delivery level is above 2.
auto WorthAttacking(Game& game, Unit const& enemy) -> bool {
  auto& data{ Data(game) };
  return opeu_unit_special(game, Near(data, enemy)) || (data.side_control[data.current_side] == SIDE_COMPUTER && data.delivery_level > 2);
}

// The engineer's order: route the unit the division last linked, else a random unit of the division; else mine a
// random free cell within three of an objective the division holds.
[[nodiscard]] auto RouteOrder(Game& game) -> bool {
  auto& data{ Data(game) };
  auto& link{ data.unit_links[data.current_side * GROUPS_PER_SIDE + data.current_group] };
  if (auto* linked{ UnitAt(data, link) }) {
    link = 0;
    if (PathOrderStep(game, *linked)) return true;
  }
  auto group_units{ GroupUnits(data, data.current_side, data.current_group) };
  if (PathOrderStep(game, group_units[game.State().RandomBounded(UNITS_PER_GROUP)])) return true;
  for (auto const& task : GroupOf(data, data.current_side, data.current_group).tasks) {
    if (task.kind == Bits(TaskKind::NONE)) continue;
    if (task.kind != Bits(TaskKind::HOLD)) continue;
    auto const* objective{ ObjectiveAt(game, task.x, task.y) };
    for (std::uint8_t attempt = 0; attempt < 10; ++attempt) {
      auto x{ static_cast<std::uint8_t>(game.State().RandomBounded(7) + objective->x) };
      auto y{ static_cast<std::uint8_t>(game.State().RandomBounded(7) + objective->y) };
      x = SubByte(x, 3);
      y = SubByte(y, 3);
      if (x >= MAP_WIDTH) continue;
      if (y >= MAP_HEIGHT) continue;
      switch (static_cast<Terrain>(opeu_map_code(game, x, y))) {
      case Terrain::IMPASSABLE:
      case Terrain::CITY:
      case Terrain::RIVER:
      case Terrain::BRIDGE:     continue;
      default:                  break;
      }
      if (opeu_cell_marked(game, { x, y }, data.current_side)) continue;
      auto const& current{ CurrentUnit(data) };
      if (x == current.x && y == current.y) continue;
      IssueOrder(game, UnitOrder::LAY_MINES, { x, y });
      return true;
    }
  }
  return false;
}

// The supply unit's order: when itself below 10% of any supply, load at the nearest overstocked city; else supply the
// nearest unit below the side's average percentages, else load at the nearest understocked or overstocked city.
[[nodiscard]] auto PlanSupply(Game& game) -> bool {
  auto&       data{ Data(game) };
  auto*       overstocked{ NearestObjective(game, Overstocked) };
  auto const& current{ CurrentUnit(data) };
  if (BelowPercent(game, current, { 10, 10, 10 })) {
    if (overstocked) {
      IssueOrder(game, UnitOrder::RESUPPLY, { overstocked->x, overstocked->y });
      return true;
    }
  } else {
    auto const* needy{ NearestUnitInNeed(game, SideResourcePercentages(game, data.current_side)) };
    if (needy && needy != &current) {
      IssueOrderAgainst(game, UnitOrder::SUPPLY_UNIT, NearOrNull(data, needy));
      return true;
    }
    if (auto const* understocked{ NearestObjective(game, Understocked) }) {
      IssueOrder(game, UnitOrder::RESUPPLY, { understocked->x, understocked->y });
      return true;
    }
    if (overstocked) {
      IssueOrder(game, UnitOrder::RESUPPLY, { overstocked->x, overstocked->y });
      return true;
    }
  }
  return false;
}

// A combat unit below 20% of any supply loads at the nearest overstocked city among its division's tasks, else takes
// an enemy city of the division's tasks that has supplies left.
[[nodiscard]] auto ResupplyOrder(Game& game) -> bool {
  auto&       data{ Data(game) };
  auto const& current{ CurrentUnit(data) };
  auto const  capacity{ UnitCapacities(data, data.current_unit_ptr) };
  if (opeu_percent(capacity.ammunition, 20) > current.resources[0] || opeu_percent(capacity.fuel, 20) > current.resources[1] || opeu_percent(capacity.food, 20) > current.resources[2]) {
    auto const* overstocked{ NearestObjective(game, Overstocked) };
    if (overstocked && GroupTargets(game, overstocked)) {
      IssueOrder(game, UnitOrder::RESUPPLY, { overstocked->x, overstocked->y });
      return true;
    }
    for (auto const& objective : data.objectives)
      if (opeu_objective_side(game, Near(data, objective)) != data.current_side && HasRemaining(game, objective) && GroupTargets(game, &objective)) {
        IssueOrder(game, UnitOrder::OCCUPY, { objective.x, objective.y });
        return true;
      }
  }
  return false;
}

// A combat unit takes the division's capture or hold task at an objective that still needs units when it is the
// division's nearest free combat unit to it; else it moves to the division's move task.
[[nodiscard]] auto TaskOrder(Game& game) -> bool {
  auto&        data{ Data(game) };
  Task const*  move_task{ nullptr };
  std::uint8_t nearest_slot{ 0 };
  for (auto const& task : GroupOf(data, data.current_side, data.current_group).tasks) {
    if (task.kind == Bits(TaskKind::NONE)) continue;
    if (task.kind == Bits(TaskKind::MOVE)) {
      move_task = &task;
      continue;
    }
    auto const* objective{ ObjectiveAt(game, task.x, task.y) };
    if (FullyAssigned(game, objective)) continue;
    std::uint8_t best{ UNREACHED };
    for (auto const [slot, unit] : std::views::zip(std::views::iota(std::uint8_t{ 0 }), GroupUnits(data, data.current_side, data.current_group))) {
      if (IsCombatSlot(slot) && (unit.flags & Bits(UnitFlag::READY)) && unit.orders[0].kind == Bits(UnitOrder::NONE)) {
        auto const distance{ GridDistance({ objective->x, objective->y }, { unit.x, unit.y }) };
        if (distance < best) {
          nearest_slot = slot;
          best         = distance;
        }
      }
    }
    if (best != UNREACHED && nearest_slot == data.current_entry) {
      UnitOrder order;
      switch (static_cast<TaskKind>(task.kind)) {
      case TaskKind::CAPTURE: order = UnitOrder::OCCUPY; break;
      case TaskKind::HOLD:    order = UnitOrder::DEFEND; break;
      default:                order = UnitOrder::NONE; break;
      }
      IssueOrder(game, order, { objective->x, objective->y });
      return true;
    }
  }
  if (move_task) {
    auto const& current{ CurrentUnit(data) };
    if (current.x != move_task->x || current.y != move_task->y) {
      IssueOrder(game, UnitOrder::MOVE, { move_task->x, move_task->y });
      return true;
    }
  }
  return false;
}

// The side's free combat unit nearest the nearest enemy unit worth attacking inside the side's influence attacks it;
// so does the current unit when such an enemy stands next to one of the division's task cells.
[[nodiscard]] auto AttackOrder(Game& game) -> bool {
  auto&        data{ Data(game) };
  auto const   side{ data.current_side };
  std::uint8_t best{ UNREACHED };
  Unit*        target{ nullptr };
  for (auto& enemy : SideUnits(data, opeu_opposite_side(side)))
    if ((enemy.flags & Bits(UnitFlag::READY)) && WorthAttacking(game, enemy) && opeu_cell_influence_side(game, { enemy.x, enemy.y }) == side) {
      auto const distance{ GridDistance({ CurrentUnit(data).x, CurrentUnit(data).y }, { enemy.x, enemy.y }) };
      if (distance < best) {
        target = &enemy;
        best   = distance;
      }
    }
  if (best != UNREACHED) {
    best = UNREACHED;
    Unit const* attacker{ nullptr };
    for (auto const& unit : SideUnits(data, side))
      if ((unit.flags & Bits(UnitFlag::READY)) && unit.orders[0].kind == Bits(UnitOrder::NONE)) {
        auto const distance{ GridDistance({ target->x, target->y }, { unit.x, unit.y }) };
        if (distance < best) {
          attacker = &unit;
          best     = distance;
        }
      }
    if (best != UNREACHED && &CurrentUnit(data) == attacker) {
      IssueOrderAgainst(game, UnitOrder::ATTACK, NearOrNull(data, target));
      return true;
    }
  }
  for (auto const& task : GroupOf(data, side, data.current_group).tasks) {
    if (task.kind == Bits(TaskKind::NONE)) continue;
    for (auto const [x, y] : CellsAround({ task.x, task.y })) {
      auto* enemy{ UnitAt(data, opeu_find_unit_at(game, x, y)) };
      if (enemy && opeu_unit_side(Near(data, *enemy)) != side && WorthAttacking(game, *enemy)) {
        IssueOrderAgainst(game, UnitOrder::ATTACK, Near(data, *enemy));
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] auto CombatOrder(Game& game) -> bool {
  return ResupplyOrder(game) || TaskOrder(game) || AttackOrder(game);
}

} // namespace

// The computer side's turn for its current unit, unless its average rating beats a roll of 80..89: the engineer
// routes, the supply unit supplies, a combat unit takes a combat order; a unit given none follows the division's
// selected unit.
void opeu_unit_task_service(Game& game) {
  auto& data{ Data(game) };
  if (game.State().RandomBounded(10) + 80 > opeu_unit_average11(game, data.current_unit_ptr)) {
    if (data.current_entry == ENGINEER_SLOT) {
      if (RouteOrder(game)) return;
    } else if (data.current_entry == SUPPLY_SLOT) {
      if (PlanSupply(game)) return;
    } else if (CombatOrder(game))
      return;
    auto const selected{ opeu_find_group_selected_unit(game, data.current_group_ptr) };
    if (selected != data.current_unit_ptr) IssueOrderAgainst(game, UnitOrder::PURSUE, selected);
  }
}
