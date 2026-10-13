// Task planning of the computer side's divisions, called through opeu_group_task_service from the turn dispatcher.
// A division gets a task for every objective linked to its side that no division covers yet and that it is the
// nearest able division to (capture an enemy objective, hold an own one); a division left without such tasks is sent
// to a random cell in a quarter of the map no other division stands in.
#include "core/groupai.hpp"
#include "map/distance.hpp"
#include "map/grid-cell.hpp"
#include "map/grid.hpp"
#include "map/query.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/orders.hpp"
#include "units/value.hpp"

#include "common/data-segment.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <ranges>
#include <span>

using namespace opeu::game;

namespace {
constexpr std::uint8_t NO_DISTANCE{ 255 };
constexpr std::uint8_t QUARTER{ 20 }; // the map's quarters are 20 x 20 cells
constexpr GridCell     MAP_CENTRE{ 20, 20 };
constexpr auto         GROUPS{ std::views::iota(std::uint8_t{ 0 }, std::uint8_t{ GROUPS_PER_SIDE }) }; // a side's divisions
constexpr std::uint8_t WEIGHT_MASK{ 0x0e };                                                            // Objective::flags bits 1-3: how many units an objective takes, less one
constexpr std::uint8_t LAST_COMBAT_SLOT{ 5 };                                                          // unit slots 0-5 of a division count as its combat units
constexpr std::uint8_t ORDER_12{ 12 };                                                                 // a unit order a replanned division also clears

[[nodiscard]] auto None(Task const& task) -> bool {
  return task.kind == Bits(TaskKind::NONE);
}

[[nodiscard]] auto Ready(Unit const& unit) -> bool {
  return unit.flags & Bits(UnitFlag::READY);
}

[[nodiscard]] auto ReadyGroup(DataSegment const& data, std::uint8_t group) -> bool {
  return GroupOf(data, data.current_side, group).flags & Bits(GroupFlag::READY);
}

[[nodiscard]] auto SameCell(Task const& task, Task const& other) -> bool {
  return task.x == other.x && task.y == other.y;
}

// The mean cell of a division's ready units; the map's centre when none is ready.
[[nodiscard]] auto GroupCenter(DataSegment& data, std::uint8_t group) -> GridCell {
  std::uint16_t sum_x{ 0 };
  std::uint16_t sum_y{ 0 };
  std::uint16_t ready{ 0 };
  for (auto const& unit : GroupUnits(data, data.current_side, group) | std::views::filter(Ready)) {
    sum_x += unit.x;
    sum_y += unit.y;
    ++ready;
  }
  if (ready == 0) return MAP_CENTRE;
  return { static_cast<std::uint8_t>(sum_x / ready), static_cast<std::uint8_t>(sum_y / ready) };
}

// Whether a division of the current side already has a task at the objective.
[[nodiscard]] auto ObjectiveTasked(Game& game, std::uint16_t objective) -> bool {
  auto& data{ Data(game) };
  return std::ranges::any_of(GROUPS, [&](std::uint8_t group) {
    return std::ranges::any_of(GroupOf(data, data.current_side, group).tasks, [&](Task const& task) { return !None(task) && opeu_find_objective_at(game, task.x, task.y) == objective; });
  });
}

// The number of units an objective takes.
[[nodiscard]] auto ObjectiveWeight(Objective const& objective) -> std::uint8_t {
  return static_cast<std::uint8_t>(((objective.flags & WEIGHT_MASK) >> 1) + 1);
}

// Whether a division's ready combat units suffice for its present tasks and the objective.
[[nodiscard]] auto GroupCanCover(Game& game, std::uint8_t group, std::uint16_t objective) -> bool {
  auto&        data{ Data(game) };
  auto const   units{ std::ranges::count_if(GroupUnits(data, data.current_side, group).first<LAST_COMBAT_SLOT + 1>(), Ready) };
  std::uint8_t needed{ ObjectiveWeight(Record<Objective>(data, objective)) };
  for (auto const& task : GroupOf(data, data.current_side, group).tasks)
    if (auto const at{ None(task) ? std::uint16_t{ 0 } : opeu_find_objective_at(game, task.x, task.y) }; at != 0) needed += ObjectiveWeight(Record<Objective>(data, at));
  return needed <= units;
}

// Whether every task of current is also among saved (the same cell).
[[nodiscard]] auto TasksRetained(std::span<Task const, 4> current, std::span<Task const, 4> saved) -> bool {
  auto const kept{ [saved](Task const& task) { return std::ranges::any_of(saved, [&task](Task const& other) { return !None(other) && SameCell(task, other); }); } };
  return std::ranges::all_of(current, [&kept](Task const& task) { return None(task) || kept(task); });
}

// The able division nearest to the objective, if any.
[[nodiscard]] auto NearestGroup(Game& game, std::uint16_t objective) -> std::optional<std::uint8_t> {
  auto&                       data{ Data(game) };
  auto const&                 target{ Record<Objective>(data, objective) };
  std::optional<std::uint8_t> nearest_group;
  std::uint8_t                nearest{ NO_DISTANCE };
  for (auto const group : GROUPS | std::views::filter([&](std::uint8_t group) { return ReadyGroup(data, group) && GroupCanCover(game, group, objective); })) {
    auto const distance{ GridDistance({ target.x, target.y }, GroupCenter(data, group)) };
    if (distance < nearest) {
      nearest_group = group;
      nearest       = distance;
    }
  }
  return nearest_group;
}

// Gives the current division a task for each linked objective it is the nearest able division to; returns the
// number of task slots now filled.
[[nodiscard]] auto AssignObjectiveTasks(Game& game) -> std::uint8_t {
  auto&        data{ Data(game) };
  std::uint8_t count{ 0 };
  auto const   links{ std::span{ data.objective_links }.subspan(data.current_side * OBJECTIVE_COUNT, OBJECTIVE_COUNT) };
  for (auto const objective : links | std::views::filter([&](std::uint16_t objective) { return objective != 0 && !ObjectiveTasked(game, objective); })) {
    if (count >= 4) break;
    if (NearestGroup(game, objective) != data.current_group) continue;
    auto const& target{ Record<Objective>(data, objective) };
    auto&       task{ GroupOf(data, data.current_side, data.current_group).tasks[count++] };
    task.x    = target.x;
    task.y    = target.y;
    task.kind = Bits(opeu_objective_side(game, objective) != data.current_side ? TaskKind::CAPTURE : TaskKind::HOLD);
  }
  return count;
}

// The map quarter (in quarter coordinates) beside or across from the current division's own that no other ready
// division stands in.
[[nodiscard]] auto FreeQuarter(DataSegment& data) -> std::optional<GridCell> {
  std::array<std::array<bool, 2>, 2> taken{};
  for (auto const group : GROUPS | std::views::filter([&](std::uint8_t group) { return ReadyGroup(data, group) && group != data.current_group; })) {
    auto const center{ GroupCenter(data, group) };
    taken[center.y / QUARTER][center.x / QUARTER] = true;
  }
  auto const own{ GroupCenter(data, data.current_group) };
  GridCell   quarter{ static_cast<std::uint8_t>(own.x / QUARTER ^ 1), static_cast<std::uint8_t>(own.y / QUARTER) };
  if (taken[quarter.y][quarter.x]) {
    quarter = { static_cast<std::uint8_t>(quarter.x ^ 1), static_cast<std::uint8_t>(quarter.y ^ 1) };
    if (taken[quarter.y][quarter.x]) return std::nullopt;
  }
  return quarter;
}

// Sends the current division to a random passable cell of a free map quarter; returns the number of task slots now
// filled.
[[nodiscard]] auto AssignQuadrantTask(Game& game) -> std::uint8_t {
  auto&      data{ Data(game) };
  auto const quarter{ FreeQuarter(data) };
  if (!quarter) return 0;
  auto const corner_x{ static_cast<std::uint8_t>((quarter->x & 1) * QUARTER) };
  auto const corner_y{ static_cast<std::uint8_t>((quarter->y & 1) * QUARTER) };
  for ([[maybe_unused]] auto const attempt : std::views::iota(0, 100)) {
    auto const cell_x{ static_cast<std::uint8_t>(game.State().RandomBounded(QUARTER) + corner_x) };
    auto const cell_y{ static_cast<std::uint8_t>(game.State().RandomBounded(QUARTER) + corner_y) };
    switch (opeu_map_code(game, cell_x, cell_y)) {
    case 0:
    case 1:
    case 5: continue; // cells a division is not sent to
    }
    auto& task{ GroupOf(data, data.current_side, data.current_group).tasks[0] };
    task.x    = cell_x;
    task.y    = cell_y;
    task.kind = Bits(TaskKind::MOVE);
    return 1;
  }
  return 0;
}
} // namespace

// Replans the current division's tasks; when they change, its combat units drop their orders.
void opeu_group_task_service(Game& game) {
  auto&                     data{ Data(game) };
  auto&                     current{ Record<Group>(data, data.current_group_ptr) };
  std::array<Task, 4> const saved{ current.tasks };
  for (auto& task : current.tasks) task.kind = Bits(TaskKind::NONE);
  auto count{ AssignObjectiveTasks(game) };
  if (count == 0) count = AssignQuadrantTask(game);
  auto& tasks{ GroupOf(data, data.current_side, data.current_group).tasks };
  for (auto& task : tasks | std::views::drop(count)) task.kind = Bits(TaskKind::NONE);
  if (TasksRetained(current.tasks, saved)) return;
  for (auto const [slot, unit] : std::views::zip(std::views::iota(0u), GroupUnits(data, data.current_side, data.current_group)))
    if (opeu_unit_selection_eligible(game, Near(data, unit)) && (slot <= LAST_COMBAT_SLOT || unit.orders[0].kind == ORDER_12)) opeu_clear_unit_orders(game, Near(data, unit), 3);
  current.flags &= static_cast<std::uint8_t>(~Bits(GroupFlag::RETARGET));
}
