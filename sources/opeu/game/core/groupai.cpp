// Task planning of the computer side's divisions, called through opeu_group_task_service from the turn dispatcher.
// A division gets a task for every objective linked to its side that no division covers yet and that it is the
// nearest able division to (capture an enemy objective, hold an own one); a division left without such tasks is sent
// to a random cell in a quarter of the map no other division stands in.
#include "core/groupai.hpp"
#include "core/random.hpp"
#include "map/distance.hpp"
#include "map/grid.hpp"
#include "map/query.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/orders.hpp"
#include "units/value.hpp"

#include "common/data-segment.hpp"
#include "common/random.hpp"

#include <array>

using namespace opeu::game;

namespace {
constexpr std::uint8_t NO_DISTANCE{ 255 };
constexpr std::uint8_t QUARTER{ 20 };         // the map's quarters are 20 x 20 cells
constexpr std::uint8_t WEIGHT_MASK{ 0x0e };   // Objective::flags bits 1-3: how many units an objective takes, less one
constexpr std::uint8_t LAST_COMBAT_SLOT{ 5 }; // unit slots 0-5 of a division count as its combat units
constexpr std::uint8_t ORDER_12{ 12 };        // a unit order a replanned division also clears

[[nodiscard]] auto None(Task const& task) -> bool {
  return task.kind == Bits(TaskKind::NONE);
}

[[nodiscard]] auto SideGroup(DataSegment& data, std::uint8_t group) -> Group& {
  return data.groups[data.current_side * GROUPS_PER_SIDE + group];
}

[[nodiscard]] auto SideUnit(DataSegment& data, std::uint8_t group, std::uint8_t slot) -> Unit& {
  return data.units[data.current_side * UNITS_PER_SIDE + group * UNITS_PER_GROUP + slot];
}

[[nodiscard]] auto Ready(Unit const& unit) -> bool {
  return unit.flags & Bits(UnitFlag::READY);
}
} // namespace

uint16_t opeu_objective_tasked(uint8_t* ds, uint16_t objective);
uint8_t  opeu_objective_weight(Objective const& objective);
uint16_t opeu_group_can_cover(uint8_t* ds, uint8_t group, uint16_t objective);

// The mean cell of a division's ready units; the map's centre when none is ready.
void opeu_group_center(uint8_t* ds, uint8_t* x, uint8_t* y, uint8_t group) {
  auto&         data{ Data(ds) };
  std::uint16_t sum_x{ 0 };
  std::uint16_t sum_y{ 0 };
  std::uint16_t ready{ 0 };
  for (std::uint8_t slot = 0; slot < UNITS_PER_GROUP; ++slot) {
    auto const& unit{ SideUnit(data, group, slot) };
    if (Ready(unit)) {
      sum_x += unit.x;
      sum_y += unit.y;
      ++ready;
    }
  }
  if (ready == 0)
    *y = *x = 20;
  else {
    *x = static_cast<std::uint8_t>(sum_x / ready);
    *y = static_cast<std::uint8_t>(sum_y / ready);
  }
}

// Whether a division of the current side already has a task at the objective.
uint16_t opeu_objective_tasked(uint8_t* ds, uint16_t objective) {
  auto& data{ Data(ds) };
  for (std::uint8_t group = 0; group < GROUPS_PER_SIDE; ++group)
    for (auto const& task : SideGroup(data, group).tasks)
      if (!None(task) && opeu_find_objective_at(ds, task.x, task.y) == objective) return 1;
  return 0;
}

// The number of units an objective takes.
uint8_t opeu_objective_weight(Objective const& objective) {
  return static_cast<std::uint8_t>(((objective.flags & WEIGHT_MASK) >> 1) + 1);
}

// Whether a division's ready combat units suffice for its present tasks and the objective.
uint16_t opeu_group_can_cover(uint8_t* ds, uint8_t group, uint16_t objective) {
  auto&        data{ Data(ds) };
  std::uint8_t units{ 0 };
  for (std::uint8_t slot = 0; slot < UNITS_PER_GROUP; ++slot)
    if (Ready(SideUnit(data, group, slot)) && slot <= LAST_COMBAT_SLOT) ++units;
  std::uint8_t needed{ opeu_objective_weight(Record<Objective>(data, objective)) };
  for (auto const& task : SideGroup(data, group).tasks)
    if (!None(task) && (objective = opeu_find_objective_at(ds, task.x, task.y)) != 0)
      needed += opeu_objective_weight(Record<Objective>(data, objective));
  return needed <= units;
}

// Whether every task of current is also among saved (the same cell).
uint16_t opeu_tasks_retained(Task const* current, Task const* saved) {
  for (std::uint8_t i = 0; i < 4; ++i, ++current) {
    if (None(*current)) continue;
    std::uint8_t j{ 0 };
    for (; j < 4; ++j) {
      auto const& task{ saved[j] };
      if (None(task)) continue;
      if (current->x == task.x && task.y == current->y) break;
    }
    if (j == 4) return 0;
  }
  return 1;
}

// Gives the current division a task for each linked objective it is the nearest able division to; returns the
// number of task slots now filled.
uint8_t opeu_assign_objective_tasks(uint8_t* ds, uint8_t count) {
  auto& data{ Data(ds) };
  for (std::uint8_t link = 0; link < OBJECTIVE_COUNT && count < 4; ++link) {
    std::uint16_t const objective{ data.objective_links[data.current_side * OBJECTIVE_COUNT + link] };
    if (!objective) continue;
    if (opeu_objective_tasked(ds, objective)) continue;
    auto const&  target{ Record<Objective>(data, objective) };
    std::uint8_t nearest_group;
    std::uint8_t nearest{ NO_DISTANCE };
    for (std::uint8_t group = 0; group < GROUPS_PER_SIDE; ++group)
      if ((SideGroup(data, group).flags & Bits(GroupFlag::READY)) && opeu_group_can_cover(ds, group, objective)) {
        std::uint8_t x;
        std::uint8_t y;
        opeu_group_center(ds, &x, &y, group);
        auto const distance{ opeu_grid_distance(target.x, target.y, x, y) };
        if (distance < nearest) {
          nearest_group = group;
          nearest       = distance;
        }
      }
    if (nearest == NO_DISTANCE) continue;
    if (data.current_group != nearest_group) continue;
    auto& task{ SideGroup(data, data.current_group).tasks[count++] };
    task.x    = target.x;
    task.y    = target.y;
    task.kind = Bits(opeu_objective_side(ds, objective) != data.current_side ? TaskKind::CAPTURE : TaskKind::HOLD);
  }
  return count;
}

// Sends the current division to a random passable cell of a map quarter beside or across from its own that no other
// ready division stands in; returns the number of task slots now filled.
uint8_t opeu_assign_quadrant_task(uint8_t* ds, uint8_t count) {
  auto&                                       data{ Data(ds) };
  std::array<std::array<std::uint16_t, 2>, 2> taken{};
  std::uint8_t                                x;
  std::uint8_t                                y;
  for (std::uint8_t group = 0; group < GROUPS_PER_SIDE; ++group)
    if ((SideGroup(data, group).flags & Bits(GroupFlag::READY)) && group != data.current_group) {
      opeu_group_center(ds, &x, &y, group);
      taken[y / QUARTER][x / QUARTER] = 1;
    }
  opeu_group_center(ds, &x, &y, data.current_group);
  x /= QUARTER;
  y /= QUARTER;
  x ^= 1;
  if (taken[y][x] != 0) {
    y ^= 1;
    x ^= 1;
    if (taken[y][x] != 0) return count;
  }
  x = static_cast<std::uint8_t>((x & 1) * QUARTER);
  y = static_cast<std::uint8_t>((y & 1) * QUARTER);
  for (std::uint8_t attempt = 0; attempt < 100; ++attempt) {
    auto const cell_x{ static_cast<std::uint8_t>(RandomBelow(ds, QUARTER) + x) };
    auto const cell_y{ static_cast<std::uint8_t>(RandomBelow(ds, QUARTER) + y) };
    switch (opeu_map_code(ds, cell_x, cell_y)) {
    case 0:
    case 1:
    case 5: continue; // cells a division is not sent to
    }
    auto& task{ SideGroup(data, data.current_group).tasks[count++] };
    task.x    = cell_x;
    task.y    = cell_y;
    task.kind = Bits(TaskKind::MOVE);
    break;
  }
  return count;
}

// Replans the current division's tasks; when they change, its combat units drop their orders.
void opeu_group_task_service(uint8_t* ds) {
  auto&                     data{ Data(ds) };
  auto&                     current{ Record<Group>(data, data.current_group_ptr) };
  std::array<Task, 4> const saved{ current.tasks };
  for (auto& task : current.tasks) task.kind = Bits(TaskKind::NONE);
  auto count{ opeu_assign_objective_tasks(ds, 0) };
  if (count == 0) count = opeu_assign_quadrant_task(ds, count);
  while (count < 4) SideGroup(data, data.current_group).tasks[count++].kind = Bits(TaskKind::NONE);
  if (!opeu_tasks_retained(current.tasks.data(), saved.data())) {
    for (std::uint8_t slot = 0; slot < UNITS_PER_GROUP; ++slot) {
      auto& unit{ SideUnit(data, data.current_group, slot) };
      if (opeu_unit_selection_eligible(ds, Near(data, unit))
          && (slot <= LAST_COMBAT_SLOT || unit.orders[0].kind == ORDER_12))
        opeu_clear_unit_orders(ds, Near(data, unit), 3);
    }
    current.flags &= static_cast<std::uint8_t>(~Bits(GroupFlag::RETARGET));
  }
}
