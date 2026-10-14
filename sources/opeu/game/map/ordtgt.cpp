// The division targets of the Orders command: the list predicates for the division rows, and setting the target of a
// division's task slot (the list row) by picking a map location. The cursor starts at the slot's target or at the
// division's selected unit; the pick is named (a friendly or enemy objective, or the cell) and confirmed, and
// cancelling clears a set slot after a confirmation. A change clears the orders of the division's units.
#include "map/ordtgt.hpp"
#include "common/data-segment.hpp"
#include "common/forces.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"
#include "common/order.hpp"
#include "core/dialogs.hpp"
#include "core/people.hpp"
#include "core/selunit.hpp"
#include "core/strings.hpp"
#include "core/textout.hpp"
#include "map/mappick.hpp"
#include "map/mappos.hpp"
#include "map/query.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/orders.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include <array>
#include <ranges>

using namespace opeu::game;

namespace {
// The DS reserve of the question text the services write, and its offset in it.
constexpr uint16_t TARGET_RESERVE{ 0x76 };
constexpr uint16_t TEXT_AT{ 0 };
constexpr uint16_t ONE_LINE_NAME{ 13 }; // longer objective names take two lines

} // namespace

// The division of the list row can be given a target when it is ready and its selected unit can be selected.
uint8_t opeu_group_target_enabled(Game& game) {
  auto&      data{ Data(game) };
  auto&      division{ GroupOf(data, data.current_side, data.supply_row) };
  auto const group{ Near(data, division) };
  return (division.flags & Bits(GroupFlag::READY)) && opeu_unit_selection_eligible(game, opeu_find_group_selected_unit(game, group));
}

uint8_t opeu_menu_always(void) {
  return 1;
}

void opeu_group_set_target(Game& game, uint8_t group) {
  auto&                      data{ Data(game) };
  TextBuffer<TARGET_RESERVE> locals;
  auto const                 text{ locals.At(TEXT_AT) };
  auto const                 person{ opeu_find_group_record(game, data.current_side, group) };
  auto&                      division{ GroupOf(data, data.current_side, group) };
  auto&                      task{ division.tasks[data.supply_row] };
  auto const                 start{ [&] {
    if (task.kind != Bits(TaskKind::NONE)) return GridCell{ task.x, task.y };
    auto const& unit{ Record<Unit>(data, opeu_find_group_selected_unit(game, Near(data, division))) };
    return GridCell{ unit.x, unit.y };
  }() };
  opeu_arrival_location(game, start.x, start.y, static_cast<uint8_t>(data.current_side));
  opeu_control_message(game, Near(data, data.target_text), person);
  bool changed{ false };
  if (auto const picked{ PickLocation(game, start) }) {
    auto const [x, y]{ *picked };
    auto const objective{ opeu_find_objective_at(game, x, y) };
    TaskKind   kind;
    if (objective) {
      kind = opeu_objective_side(game, objective) != data.current_side ? TaskKind::CAPTURE : TaskKind::HOLD;
      auto const format{ opeu_string_length(game, objective) > ONE_LINE_NAME ? Near(data, data.target_two_line_format) : Near(data, data.target_one_line_format) };
      opeu_request_format(game, text, std::array<FormatArgument, 3>{ format, data.order_names[Bits(kind)], objective });
    } else {
      kind = TaskKind::MOVE;
      opeu_request_format(game, text, std::array<FormatArgument, 3>{ Near(data, data.target_cell_format), data.map_scale * x, y * data.map_scale });
    }
    if (opeu_confirm_message(game, text, person)) {
      task.x    = x;
      task.y    = y;
      task.kind = Bits(kind);
      changed   = true;
    }
  } else if (task.kind != Bits(TaskKind::NONE) && opeu_confirm_message(game, Near(data, data.target_cancel_text), person)) {
    task.kind = Bits(TaskKind::NONE);
    changed   = true;
  }
  if (!changed) return;
  auto const first{ (data.current_side * GROUPS_PER_SIDE + group) * UNITS_PER_GROUP };
  for (auto const i : std::views::iota(unsigned{ 0 }, UNITS_PER_GROUP)) opeu_clear_unit_orders(game, Near(data, data.units[first + i]), ALL_ORDER_SLOTS);
}
