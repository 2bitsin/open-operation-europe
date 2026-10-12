// The computer side's orders for its current unit: engineers route around mines and rivers and mine near the
// objectives their division holds, supply units carry supplies between cities and units, and combat units take or hold
// their division's objectives, resupply and attack enemy units within reach. Paths come from a flood fill of step
// costs over the map.
#include "types.hpp"
#include "core/unitai.hpp"
#include "core/aiterr.hpp"
#include "core/limits.hpp"
#include "core/recindex.hpp"
#include "core/numeric.hpp"
#include "core/selunit.hpp"
#include "core/sideutil.hpp"
#include "map/cellflag.hpp"
#include "map/distance.hpp"
#include "map/grid.hpp"
#include "map/objeleg.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/support.hpp"
#include "units/aggpct.hpp"
#include "units/capacity.hpp"
#include "units/orders.hpp"
#include "units/stats.hpp"
#include "units/value.hpp"

#include "common/data-segment.hpp"
#include "common/forces.hpp"
#include "common/random.hpp"

#include <array>
#include <cstdint>

using namespace opeu::game;

namespace {

constexpr std::uint8_t UNREACHED{ 0xff }; // a distance or step cost no cell has
constexpr std::uint16_t UNVISITED{
  0xffff
}; // the path cost of a cell the fill has not reached
constexpr std::uint8_t IMPASSABLE_STEP{
  255
}; // the step cost of a cell the unit cannot enter
constexpr std::uint8_t DIRECTIONS{ 8 };
constexpr std::uint8_t ENGINEER_SLOT{ 6 }; // the engineer unit of a division
constexpr std::uint8_t SUPPLY_SLOT{ 7 };   // the supply unit of a division

// The first six unit slots of a division are its combat units.
constexpr auto IsCombatSlot(std::uint8_t slot) -> bool {
  return slot <= 5;
}

// The flood fill's queue of cell indices, kept sorted by path cost, and the path cost of every cell.
std::array<std::uint16_t, MAP_WIDTH * MAP_HEIGHT> path_queue;
std::array<std::uint16_t, MAP_WIDTH * MAP_HEIGHT> path_cost;

auto Distance1D(int difference) -> int {
  return static_cast<int16_t>(
      opeu_abs16_bits(static_cast<std::uint16_t>(difference)));
}

auto CurrentUnit(DataSegment& data) -> Unit& {
  return Record<Unit>(data, data.current_unit_ptr);
}

// The near pointer of a record, 0 for none.
template <typename T>
auto NearOrNull(DataSegment const& data, T const* record) -> std::uint16_t {
  return record ? Near(data, *record) : 0;
}

auto UnitAt(DataSegment& data, std::uint16_t near) -> Unit* {
  return near ? &Record<Unit>(data, near) : nullptr;
}

auto ObjectiveAt(std::uint8_t* ds, std::uint8_t x, std::uint8_t y)
    -> Objective* {
  auto const near{ opeu_find_objective_at(ds, x, y) };
  return near ? &Record<Objective>(Data(ds), near) : nullptr;
}

// Gives the current unit an order at a cell or against a target record.
void IssueOrder(std::uint8_t* ds, UnitOrder kind, std::uint8_t x,
                std::uint8_t y, std::uint16_t target = 0) {
  opeu_set_unit_order(ds, Data(ds).current_unit_ptr, target, x, 0, Bits(kind),
                      y);
}

// Fills path_cost outward from (x, y) in order of cost, each step costing the unit's terrain rating of the cell
// entered. Returns 1 as soon as a step reaches (tx, ty), 0 when every reachable cell is costed.
auto PathFill(std::uint8_t* ds, std::uint8_t x, std::uint8_t y, std::uint8_t tx,
              std::uint8_t ty, Unit const& unit) -> std::uint16_t {
  auto const&   data{ Data(ds) };
  std::uint16_t head{ 0 };
  std::uint16_t tail{ 1 };
  path_queue[head] = static_cast<std::uint16_t>(x + y * MAP_WIDTH);
  path_cost.fill(UNVISITED);
  path_cost[path_queue[head]] = 0;
  while (tail != head) {
    std::uint8_t const  cx{ static_cast<std::uint8_t>(path_queue[head] %
                                                      MAP_WIDTH) };
    std::uint8_t const  cy{ static_cast<std::uint8_t>(path_queue[head] /
                                                      MAP_WIDTH) };
    std::uint16_t const cost{ path_cost[cy * MAP_WIDTH + cx] };
    for (std::uint8_t direction = 0; direction < DIRECTIONS; direction++) {
      auto nx{ cx };
      auto ny{ cy };
      if (!opeu_grid_step(&nx, &ny, direction))
        continue;
      if (!opeu_map_code(ds, nx, ny))
        continue;
      if (nx == tx && ny == ty)
        return 1;
      auto const cell{ static_cast<std::uint16_t>(ny * MAP_WIDTH + nx) };
      auto&      entered{ path_cost[cell] };
      if (entered == UNVISITED) {
        auto const step{ opeu_best_terrain(ds, nx, ny, Near(data, unit)) };
        if (step == IMPASSABLE_STEP)
          continue;
        entered = static_cast<std::uint16_t>(step + cost);
        // Insert from the tail: cells of equal cost stay in arrival order.
        std::uint16_t slot{ tail };
        std::uint16_t before{ static_cast<std::uint16_t>(tail - 1) };
        for (;;) {
          path_queue[slot] = path_queue[before];
          if (path_cost[path_queue[before]] <= entered) {
            path_queue[slot] = cell;
            tail++;
            break;
          }
          slot = before--;
        }
      }
    }
    head++;
  }
  return 0;
}

// Routes the unit towards its order's destination: when the path down the cost grid runs into a cell marked by the
// side, the unit clears the mines there; into a river, it bridges it.
auto PathOrderStep(std::uint8_t* ds, Unit& unit) -> std::uint16_t {
  auto& data{ Data(ds) };
  if (!(unit.flags & Bits(UnitFlag::READY)))
    return 0;
  std::uint8_t tx;
  std::uint8_t ty;
  if (!opeu_unit_order_destination(ds, Near(data, unit), &tx, &ty))
    return 0;
  auto x{ unit.x };
  auto y{ unit.y };
  PathFill(ds, tx, ty, x, y, unit);
  if (!opeu_path_descend(ds, &x, &y, tx, ty))
    return 0;
  if (opeu_cell_marked(ds, x, y, data.current_side))
    IssueOrder(ds, UnitOrder::CLEAR_MINES, x, y);
  else if (opeu_map_code(ds, x, y) == Bits(Terrain::RIVER))
    IssueOrder(ds, UnitOrder::BUILD_BRIDGE, x, y);
  return 1;
}

// Whether the objective holds more (Overstocked) or less (Understocked) of any supply than its reserve.
auto Overstocked(Objective const& objective) -> bool {
  return objective.resources[0] > objective.reserves[0] ||
         objective.resources[1] > objective.reserves[1] ||
         objective.resources[2] > objective.reserves[2];
}

auto Understocked(Objective const& objective) -> bool {
  return objective.resources[0] < objective.reserves[0] ||
         objective.resources[1] < objective.reserves[1] ||
         objective.resources[2] < objective.reserves[2];
}

// The side's eligible objective nearest the current unit that satisfies the predicate.
template <typename Predicate>
auto NearestObjective(std::uint8_t* ds, Predicate predicate) -> Objective* {
  auto&        data{ Data(ds) };
  auto const&  current{ CurrentUnit(data) };
  Objective*   nearest{ nullptr };
  std::uint8_t best{ UNREACHED };
  for (auto& objective : data.objectives)
    if (objective.active &&
        opeu_objective_side(ds, Near(data, objective)) == data.current_side &&
        opeu_objective_eligible(ds, Near(data, objective))) {
      auto const distance{ opeu_grid_distance(current.x, current.y, objective.x,
                                              objective.y) };
      if (predicate(objective) && distance < best) {
        nearest = &objective;
        best    = distance;
      }
    }
  return nearest;
}

// Whether any of the unit's supplies is below its percentage of the unit's capacity.
auto BelowPercent(std::uint8_t* ds, Unit const& unit, std::uint16_t ammo,
                  std::uint16_t fuel, std::uint16_t food) -> bool {
  std::uint16_t ammo_capacity;
  std::uint16_t fuel_capacity;
  std::uint16_t food_capacity;
  opeu_unit_capacities(ds, Near(Data(ds), unit), &ammo_capacity, &fuel_capacity,
                       &food_capacity);
  return opeu_mul_div(unit.resources[0], 100, ammo_capacity) < ammo ||
         opeu_mul_div(unit.resources[1], 100, fuel_capacity) < fuel ||
         opeu_mul_div(unit.resources[2], 100, food_capacity) < food;
}

// The supported unit of the current division nearest the current unit with a supply below the given percentages.
auto NearestUnitInNeed(std::uint8_t* ds, std::uint16_t ammo, std::uint16_t fuel,
                       std::uint16_t food) -> Unit* {
  auto&        data{ Data(ds) };
  auto const&  current{ CurrentUnit(data) };
  Unit*        nearest{ nullptr };
  std::uint8_t best{ UNREACHED };
  for (auto& unit : GroupUnits(data, data.current_side, data.current_group))
    if ((unit.flags & Bits(UnitFlag::READY)) && &unit != &current &&
        opeu_unit_support_condition(ds, Near(data, unit))) {
      auto const distance{ opeu_grid_distance(current.x, current.y, unit.x,
                                              unit.y) };
      if (BelowPercent(ds, unit, ammo, fuel, food) && distance < best) {
        nearest = &unit;
        best    = distance;
      }
    }
  return nearest;
}

// Whether the objective has supplies to take: any at all when it is the enemy's, any above its reserve when it is
// the side's own.
auto HasRemaining(std::uint8_t* ds, Objective const& objective) -> bool {
  auto const& data{ Data(ds) };
  auto        ammo{ objective.resources[0] };
  auto        fuel{ objective.resources[1] };
  auto        food{ objective.resources[2] };
  if (opeu_objective_side(ds, Near(data, objective)) == data.current_side) {
    opeu_sub_word(objective.reserves[0], &ammo);
    opeu_sub_word(objective.reserves[1], &fuel);
    opeu_sub_word(objective.reserves[2], &food);
  }
  return ammo || fuel || food;
}

// Whether one of the current division's tasks lies at the objective.
auto GroupTargets(std::uint8_t* ds, Objective const* objective) -> bool {
  auto const& data{ Data(ds) };
  for (auto const& task :
       GroupOf(data, data.current_side, data.current_group).tasks)
    if (task.kind != Bits(TaskKind::NONE)) {
      auto const* at{ ObjectiveAt(ds, task.x, task.y) };
      if (at && at == objective)
        return true;
    }
  return false;
}

// The combat units an objective needs: one, plus one for every two levels.
auto RequiredUnits(Objective const& objective) -> std::uint8_t {
  return static_cast<std::uint8_t>(
      ((objective.flags & Bits(ObjectiveFlag::LEVEL_MASK)) >> 1) + 1);
}

// Whether enough combat units of the current division already have an order at the objective.
auto FullyAssigned(std::uint8_t* ds, Objective const* objective) -> bool {
  auto&        data{ Data(ds) };
  std::uint8_t assigned{ 0 };
  std::uint8_t slot{ 0 };
  for (auto const& unit :
       GroupUnits(data, data.current_side, data.current_group)) {
    if (IsCombatSlot(slot)) {
      auto const& order{ unit.orders[0] };
      if ((unit.flags & Bits(UnitFlag::READY)) &&
          order.kind != Bits(UnitOrder::NONE) &&
          ObjectiveAt(ds, order.cell.x, order.cell.y) == objective)
        ++assigned;
    }
    ++slot;
  }
  return RequiredUnits(*objective) <= assigned;
}

// Whether the enemy unit is worth attacking: a special unit, or any unit while the side controls nothing and the
// delivery level is above 2.
auto WorthAttacking(std::uint8_t* ds, Unit const& enemy) -> bool {
  auto& data{ Data(ds) };
  return opeu_unit_special(ds, Near(data, enemy)) ||
         (data.side_control[data.current_side] == 0 && data.delivery_level > 2);
}

} // namespace

// Walks from (x, y) down the path cost grid to (tx, ty), each step to the cheapest neighbour (on a tie, one no
// farther from the target than the start was). Stops and returns 1 on a cell the side has marked or on a river.
uint16_t opeu_path_descend(uint8_t ds[65536], uint8_t* x, uint8_t* y,
                           uint8_t tx, uint8_t ty) {
  auto const& data{ Data(ds) };
  auto const  start_dx{ Distance1D(*x - tx) };
  auto const  start_dy{ Distance1D(*y - ty) };
  while (*x != tx || *y != ty) {
    std::uint16_t best{ UNVISITED };
    auto          bx{ *x };
    auto          by{ *y };
    for (std::uint8_t direction = 0; direction < DIRECTIONS; ++direction) {
      auto nx{ *x };
      auto ny{ *y };
      if (opeu_grid_step(&nx, &ny, direction)) {
        auto const cost{ path_cost[ny * MAP_WIDTH + nx] };
        if (cost < best || (cost == best && Distance1D(nx - tx) <= start_dx &&
                            Distance1D(ny - ty) <= start_dy)) {
          bx   = nx;
          by   = ny;
          best = cost;
        }
      }
    }
    *x = bx;
    *y = by;
    if (opeu_cell_marked(ds, bx, by, data.current_side) ||
        opeu_map_code(ds, bx, by) == Bits(Terrain::RIVER))
      return 1;
  }
  return 0;
}

// The engineer's order: route the unit the division last linked, else a random unit of the division; else mine a
// random free cell within three of an objective the division holds.
uint16_t opeu_unit_route_order(uint8_t ds[65536]) {
  auto& data{ Data(ds) };
  auto& link{
    data.unit_links[data.current_side * GROUPS_PER_SIDE + data.current_group]
  };
  if (auto* linked{ UnitAt(data, link) }) {
    link = 0;
    if (PathOrderStep(ds, *linked))
      return 1;
  }
  auto group_units{ GroupUnits(data, data.current_side, data.current_group) };
  if (PathOrderStep(ds, group_units[RandomBelow(ds, UNITS_PER_GROUP)]))
    return 1;
  for (auto const& task :
       GroupOf(data, data.current_side, data.current_group).tasks) {
    if (task.kind == Bits(TaskKind::NONE))
      continue;
    if (task.kind != Bits(TaskKind::HOLD))
      continue;
    auto const* objective{ ObjectiveAt(ds, task.x, task.y) };
    for (std::uint8_t attempt = 0; attempt < 10; ++attempt) {
      auto x{ static_cast<std::uint8_t>(RandomBelow(ds, 7) + objective->x) };
      auto y{ static_cast<std::uint8_t>(RandomBelow(ds, 7) + objective->y) };
      opeu_sub_byte(3, &x);
      opeu_sub_byte(3, &y);
      if (x >= MAP_WIDTH)
        continue;
      if (y >= MAP_HEIGHT)
        continue;
      switch (static_cast<Terrain>(opeu_map_code(ds, x, y))) {
        case Terrain::IMPASSABLE:
        case Terrain::CITY:
        case Terrain::RIVER:
        case Terrain::BRIDGE:     continue;
        default:                  break;
      }
      if (opeu_cell_marked(ds, x, y, data.current_side))
        continue;
      auto const& current{ CurrentUnit(data) };
      if (x == current.x && y == current.y)
        continue;
      IssueOrder(ds, UnitOrder::LAY_MINES, x, y);
      return 1;
    }
  }
  return 0;
}

// The supply unit's order: when itself below 10% of any supply, load at the nearest overstocked city; else supply the
// nearest unit below the side's average percentages, else load at the nearest understocked or overstocked city.
uint16_t opeu_plan_supply(uint8_t ds[65536]) {
  auto&       data{ Data(ds) };
  auto*       overstocked{ NearestObjective(ds, Overstocked) };
  auto const& current{ CurrentUnit(data) };
  if (BelowPercent(ds, current, 10, 10, 10)) {
    if (overstocked) {
      IssueOrder(ds, UnitOrder::RESUPPLY, overstocked->x, overstocked->y);
      return 1;
    }
  } else {
    std::uint16_t ammo;
    std::uint16_t fuel;
    std::uint16_t food;
    opeu_side_resource_percentages(ds, data.current_side, &ammo, &fuel, &food,
                                   0);
    auto const* needy{ NearestUnitInNeed(ds, ammo, fuel, food) };
    if (needy && needy != &current) {
      IssueOrder(ds, UnitOrder::SUPPLY_UNIT, 0, 0, NearOrNull(data, needy));
      return 1;
    }
    if (auto const* understocked{ NearestObjective(ds, Understocked) }) {
      IssueOrder(ds, UnitOrder::RESUPPLY, understocked->x, understocked->y);
      return 1;
    }
    if (overstocked) {
      IssueOrder(ds, UnitOrder::RESUPPLY, overstocked->x, overstocked->y);
      return 1;
    }
  }
  return 0;
}

// A combat unit below 20% of any supply loads at the nearest overstocked city among its division's tasks, else takes
// an enemy city of the division's tasks that has supplies left.
uint16_t opeu_unit_resupply_order(uint8_t ds[65536]) {
  auto&         data{ Data(ds) };
  auto const&   current{ CurrentUnit(data) };
  std::uint16_t ammo_capacity;
  std::uint16_t fuel_capacity;
  std::uint16_t food_capacity;
  opeu_unit_capacities(ds, data.current_unit_ptr, &ammo_capacity,
                       &fuel_capacity, &food_capacity);
  if (opeu_percent(ammo_capacity, 20) > current.resources[0] ||
      opeu_percent(fuel_capacity, 20) > current.resources[1] ||
      opeu_percent(food_capacity, 20) > current.resources[2]) {
    auto const* overstocked{ NearestObjective(ds, Overstocked) };
    if (overstocked && GroupTargets(ds, overstocked)) {
      IssueOrder(ds, UnitOrder::RESUPPLY, overstocked->x, overstocked->y);
      return 1;
    }
    for (auto const& objective : data.objectives)
      if (opeu_objective_side(ds, Near(data, objective)) != data.current_side &&
          HasRemaining(ds, objective) && GroupTargets(ds, &objective)) {
        IssueOrder(ds, UnitOrder::OCCUPY, objective.x, objective.y);
        return 1;
      }
  }
  return 0;
}

// A combat unit takes the division's capture or hold task at an objective that still needs units when it is the
// division's nearest free combat unit to it; else it moves to the division's move task.
uint16_t opeu_unit_task_order(uint8_t ds[65536]) {
  auto&        data{ Data(ds) };
  Task const*  move_task{ nullptr };
  std::uint8_t nearest_slot{ 0 };
  for (auto const& task :
       GroupOf(data, data.current_side, data.current_group).tasks) {
    if (task.kind == Bits(TaskKind::NONE))
      continue;
    if (task.kind == Bits(TaskKind::MOVE)) {
      move_task = &task;
      continue;
    }
    auto const* objective{ ObjectiveAt(ds, task.x, task.y) };
    if (FullyAssigned(ds, objective))
      continue;
    std::uint8_t best{ UNREACHED };
    std::uint8_t slot{ 0 };
    for (auto const& unit :
         GroupUnits(data, data.current_side, data.current_group)) {
      if (IsCombatSlot(slot) && (unit.flags & Bits(UnitFlag::READY)) &&
          unit.orders[0].kind == Bits(UnitOrder::NONE)) {
        auto const distance{ opeu_grid_distance(objective->x, objective->y,
                                                unit.x, unit.y) };
        if (distance < best) {
          nearest_slot = slot;
          best         = distance;
        }
      }
      ++slot;
    }
    if (best != UNREACHED && nearest_slot == data.current_entry) {
      UnitOrder order;
      switch (static_cast<TaskKind>(task.kind)) {
        case TaskKind::CAPTURE: order = UnitOrder::OCCUPY; break;
        case TaskKind::HOLD:    order = UnitOrder::DEFEND; break;
        default:                order = UnitOrder::NONE; break;
      }
      IssueOrder(ds, order, objective->x, objective->y);
      return 1;
    }
  }
  if (move_task) {
    auto const& current{ CurrentUnit(data) };
    if (current.x != move_task->x || current.y != move_task->y) {
      IssueOrder(ds, UnitOrder::MOVE, move_task->x, move_task->y);
      return 1;
    }
  }
  return 0;
}

// The side's free combat unit nearest the nearest enemy unit worth attacking inside the side's influence attacks it;
// so does the current unit when such an enemy stands next to one of the division's task cells.
uint16_t opeu_unit_attack_order(uint8_t ds[65536]) {
  auto&        data{ Data(ds) };
  auto const   side{ data.current_side };
  std::uint8_t best{ UNREACHED };
  Unit*        target{ nullptr };
  for (auto& enemy : SideUnits(data, opeu_opposite_side(side)))
    if ((enemy.flags & Bits(UnitFlag::READY)) && WorthAttacking(ds, enemy) &&
        opeu_cell_influence_side(ds, enemy.x, enemy.y) == side) {
      auto const distance{ opeu_grid_distance(
          CurrentUnit(data).x, CurrentUnit(data).y, enemy.x, enemy.y) };
      if (distance < best) {
        target = &enemy;
        best   = distance;
      }
    }
  if (best != UNREACHED) {
    best = UNREACHED;
    Unit const* attacker{ nullptr };
    for (auto const& unit : SideUnits(data, side))
      if ((unit.flags & Bits(UnitFlag::READY)) &&
          unit.orders[0].kind == Bits(UnitOrder::NONE)) {
        auto const distance{ opeu_grid_distance(target->x, target->y, unit.x,
                                                unit.y) };
        if (distance < best) {
          attacker = &unit;
          best     = distance;
        }
      }
    if (best != UNREACHED && &CurrentUnit(data) == attacker) {
      IssueOrder(ds, UnitOrder::ATTACK, 0, 0, NearOrNull(data, target));
      return 1;
    }
  }
  for (auto const& task : GroupOf(data, side, data.current_group).tasks) {
    if (task.kind == Bits(TaskKind::NONE))
      continue;
    for (std::uint8_t direction = 0; direction < DIRECTIONS; ++direction) {
      auto x{ task.x };
      auto y{ task.y };
      if (!opeu_grid_step(&x, &y, direction))
        continue;
      auto* enemy{ UnitAt(data, opeu_find_unit_at(ds, x, y)) };
      if (enemy && opeu_unit_side(Near(data, *enemy)) != side &&
          WorthAttacking(ds, *enemy)) {
        IssueOrder(ds, UnitOrder::ATTACK, 0, 0, Near(data, *enemy));
        return 1;
      }
    }
  }
  return 0;
}

uint16_t opeu_unit_combat_order(uint8_t ds[65536]) {
  if (opeu_unit_resupply_order(ds) || opeu_unit_task_order(ds) ||
      opeu_unit_attack_order(ds))
    return 1;
  return 0;
}

// The computer side's turn for its current unit, unless its average rating beats a roll of 80..89: the engineer
// routes, the supply unit supplies, a combat unit takes a combat order; a unit given none follows the division's
// selected unit.
void opeu_unit_task_service(uint8_t ds[65536]) {
  auto& data{ Data(ds) };
  if (RandomBelow(ds, 10) + 80 >
      opeu_unit_average11(ds, data.current_unit_ptr)) {
    if (data.current_entry == ENGINEER_SLOT) {
      if (opeu_unit_route_order(ds))
        return;
    } else if (data.current_entry == SUPPLY_SLOT) {
      if (opeu_plan_supply(ds))
        return;
    } else if (opeu_unit_combat_order(ds))
      return;
    auto const selected{ opeu_find_group_selected_unit(
        ds, data.current_group_ptr) };
    if (selected != data.current_unit_ptr)
      IssueOrder(ds, UnitOrder::PURSUE, 0, 0, selected);
  }
}
