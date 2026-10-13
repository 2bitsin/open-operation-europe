// The computer side's division tasks: dropping tasks that no longer apply, and whether the division or the current
// unit has task work to do this turn.
#include "core/grptask.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "core/groupai.hpp"
#include "core/selunit.hpp"
#include "core/unitai.hpp"
#include "map/query.hpp"
#include "map/support.hpp"
#include "port.hpp"

using namespace opeu::game;

namespace {

// The services the turn predicates call through their event.
enum Service : std::uint16_t {
  GROUP_TASKS = 0, // plan the division's tasks
  UNIT_TASKS  = 1, // give the current unit an order from its division's tasks
};

constexpr std::uint8_t NO_ORDER{ 0xff };

auto CurrentGroup(DataSegment& data) -> Group& {
  return Record<Group>(data, data.current_group_ptr);
}
} // namespace

// Drops each capture task whose objective is gone or already the side's, and each hold task whose objective is gone
// or no longer the side's.
void opeu_clean_group_tasks(Game& game) {
  auto& data{ Data(game) };
  for (auto& task : CurrentGroup(data).tasks) {
    if (task.kind == Bits(TaskKind::NONE)) continue;
    auto const objective{ opeu_find_objective_at(game, task.x, task.y) };
    if (task.kind == Bits(TaskKind::CAPTURE)) {
      if (!objective || opeu_objective_side(game, objective) == data.current_side) task.kind = Bits(TaskKind::NONE);
    } else if (task.kind == Bits(TaskKind::HOLD)) {
      if (!objective || opeu_objective_side(game, objective) != data.current_side) task.kind = Bits(TaskKind::NONE);
    }
  }
}

// Whether any of the current side's entry values is set.
uint16_t opeu_side_has_entries(Game& game) {
  auto const& data{ Data(game) };
  for (std::uint8_t i{ 0 }; i < OBJECTIVE_LINKS_PER_SIDE; ++i)
    if (data.side_entry_values[data.current_side * OBJECTIVE_LINKS_PER_SIDE + i]) return 1;
  return 0;
}

// Whether the current division holds a task (a division about to plan its tasks again holds none).
uint16_t opeu_group_has_tasks(Game& game) {
  auto& group{ CurrentGroup(Data(game)) };
  if (!(group.flags & Bits(GroupFlag::RETARGET)))
    for (auto const& task : group.tasks)
      if (task.kind != Bits(TaskKind::NONE)) return 1;
  return 0;
}

// On a computer-controlled side: plans the division's tasks when its selected unit can act, the side has entries and
// the division has no task yet.
void opeu_turn_group_action(Game& game, uint16_t (*event)(Game&, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t)) {
  auto& data{ Data(game) };
  opeu_clean_group_tasks(game);
  if (data.side_control[data.current_side] == SIDE_COMPUTER)
    if (opeu_unit_selection_eligible(game, opeu_find_group_selected_unit(game, data.current_group_ptr)) && opeu_side_has_entries(game) && !opeu_group_has_tasks(game))
      event(game, GROUP_TASKS, 0, 0, 0, 0);
}

// Whether the current unit already has an order; if not and its division is marked by the computer side and holds a
// task, the unit is given one from the tasks (and the predicate still answers 0).
uint16_t opeu_turn_final_predicate(Game& game, uint16_t (*event)(Game&, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t)) {
  auto& data{ Data(game) };
  if (opeu_unit_selection_eligible(game, data.current_unit_ptr)) {
    if (Record<Unit>(data, data.current_unit_ptr).orders[0].kind != NO_ORDER) return 1;
    auto const& group{ CurrentGroup(data) };
    if (group.flags & Bits(GroupFlag::AI_MARK))
      for (auto const& task : group.tasks)
        if (task.kind != Bits(TaskKind::NONE)) {
          event(game, UNIT_TASKS, 0, 0, 0, 0);
          break;
        }
  }
  return 0;
}

uint16_t svc_grptask(Game& game, uint16_t id, uint16_t, uint16_t, uint16_t, uint16_t) {
  switch (id) {
  case GROUP_TASKS: opeu_group_task_service(game); break;
  case UNIT_TASKS:  opeu_unit_task_service(game); break;
  default:          port_unhandled("grptask", id, 4);
  }
  return 0;
}
