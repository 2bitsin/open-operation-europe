// The division targets of the Orders command: the list predicates for the division rows, and setting the target of a
// division's task slot (the list row) by picking a map location. The cursor starts at the slot's target or at the
// division's selected unit; the pick is named (a friendly or enemy objective, or the cell) and confirmed, and
// cancelling clears a set slot after a confirmation. A change clears the orders of the division's units.
#include "map/ordtgt.hpp"
#include "common/data-segment.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"
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

#include "glue.hpp"
#include "port.hpp"

using namespace opeu::game;

namespace {
enum class TargetService : uint16_t {
  FORMAT           = 0, // buffer, format, words...
  CONTROL_MESSAGE  = 1, // text, person
  CONFIRM          = 2, // text, person
  ARRIVAL_LOCATION = 3, // x, y, side
  PICK_LOCATION    = 4, // DS offsets of x and y
};
// Below the frame (entry SP - 2): the picked cell and the question text.
constexpr uint16_t PICKED_X_AT{ 0x0c };
constexpr uint16_t PICKED_Y_AT{ 0x0e };
constexpr uint16_t TEXT_AT{ 0x76 };
constexpr uint16_t ONE_LINE_NAME{ 13 }; // longer objective names take two lines
constexpr uint8_t  ORDER_SLOTS{ 3 };    // the order slots cleared per unit

[[nodiscard]] auto DivisionOf(DataSegment& data, uint8_t group) -> Group& {
  return data.groups[data.current_side * GROUPS_PER_SIDE + group];
}
} // namespace

// The division of the list row can be given a target when it is ready and its selected unit can be selected.
uint8_t opeu_group_target_enabled(uint8_t* ds) {
  auto&      data{ Data(ds) };
  auto&      division{ DivisionOf(data, data.supply_row) };
  auto const group{ Near(data, division) };
  return (division.flags & Bits(GroupFlag::READY)) && opeu_unit_selection_eligible(ds, opeu_find_group_selected_unit(ds, group));
}

uint8_t opeu_menu_always(void) {
  return 1;
}

// sp: the entry SP; the routine's locals lie below sp - 2.
void opeu_group_set_target(uint8_t* ds, HostEvent event, uint16_t sp, uint8_t group) {
  auto&      data{ Data(ds) };
  auto const frame{ static_cast<uint16_t>(sp - 2) };
  auto const x_at{ Local(frame, PICKED_X_AT) };
  auto const y_at{ Local(frame, PICKED_Y_AT) };
  auto const text{ Local(frame, TEXT_AT) };
  auto const person{ opeu_find_group_record(ds, data.current_side, group) };
  auto&      division{ DivisionOf(data, group) };
  auto&      task{ division.tasks[data.supply_row] };
  if (task.kind != Bits(TaskKind::NONE)) {
    Byte(ds, x_at) = task.x;
    Byte(ds, y_at) = task.y;
  } else {
    auto const& unit{ Record<Unit>(data, opeu_find_group_selected_unit(ds, Near(data, division))) };
    Byte(ds, x_at) = unit.x;
    Byte(ds, y_at) = unit.y;
  }
  Call(ds, event, TargetService::ARRIVAL_LOCATION, Byte(ds, x_at), Byte(ds, y_at), data.current_side);
  Call(ds, event, TargetService::CONTROL_MESSAGE, Near(data, data.target_text), person);
  bool changed{ false };
  if (Call(ds, event, TargetService::PICK_LOCATION, x_at, y_at)) {
    auto const x{ Byte(ds, x_at) };
    auto const y{ Byte(ds, y_at) };
    auto const objective{ opeu_find_objective_at(ds, x, y) };
    TaskKind   kind;
    if (objective) {
      kind = opeu_objective_side(ds, objective) != data.current_side ? TaskKind::CAPTURE : TaskKind::HOLD;
      auto const format{ opeu_string_length(ds, objective) > ONE_LINE_NAME ? Near(data, data.target_two_line_format) : Near(data, data.target_one_line_format) };
      Call(ds, event, TargetService::FORMAT, text, format, data.order_names[Bits(kind)], objective);
    } else {
      kind = TaskKind::MOVE;
      Call(ds, event, TargetService::FORMAT, text, Near(data, data.target_cell_format), data.map_scale * x, y * data.map_scale);
    }
    if (Call(ds, event, TargetService::CONFIRM, text, person)) {
      task.x    = Byte(ds, x_at);
      task.y    = Byte(ds, y_at);
      task.kind = Bits(kind);
      changed   = true;
    }
  } else if (task.kind != Bits(TaskKind::NONE) && Call(ds, event, TargetService::CONFIRM, Near(data, data.target_cancel_text), person)) {
    task.kind = Bits(TaskKind::NONE);
    changed   = true;
  }
  if (!changed) return;
  auto const first{ (data.current_side * GROUPS_PER_SIDE + group) * UNITS_PER_GROUP };
  for (unsigned i{ 0 }; i < UNITS_PER_GROUP; i++) opeu_clear_unit_orders(ds, Near(data, data.units[first + i]), ORDER_SLOTS);
}

uint16_t svc_ordtgt(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  auto const arg{ [&](unsigned i) { return Arg(args, count, i); } };
  auto const host{ [] { return static_cast<DialogHost const*>(port_dialogs_host()); } };
  switch (static_cast<TargetService>(id)) {
  case TargetService::FORMAT:          opeu_request_format(ds, { args, count }); return 0;
  case TargetService::CONTROL_MESSAGE: opeu_control_message(ds, host(), arg(0), arg(1)); return 0;
  case TargetService::CONFIRM:         return opeu_confirm_message(ds, host(), arg(0), arg(1));
  case TargetService::ARRIVAL_LOCATION:
    WithMapFrame([&](uint16_t frame) { opeu_arrival_location(ds, svc_mappos, frame, static_cast<uint8_t>(arg(0)), static_cast<uint8_t>(arg(1)), static_cast<uint8_t>(arg(2))); });
    return 0;
  case TargetService::PICK_LOCATION: return WithMapFrame([&](uint16_t frame) { return opeu_map_pick_location(ds, svc_mappick, frame, arg(0), arg(1)); });
  default:                           port_unhandled("ordtgt", id, count);
  }
  return 0;
}

auto port_routine_opeu_group_target_enabled(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_group_target_enabled(ds);
}

auto port_routine_opeu_menu_always(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)ds;
  (void)args;
  (void)count;
  return opeu_menu_always();
}
