// Checks of a movement order's target (a cell next to the current unit, a friendly objective, a unit the side can
// see) and the order progress counters, plus the movement routines' shared services.
#include "map/movetgt.hpp"
#include "common/data-segment.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/recindex.hpp"
#include "map/cellflag.hpp"
#include "map/distance.hpp"
#include "map/maperase.hpp"
#include "map/mappos.hpp"
#include "map/mapunit.hpp"
#include "map/movecost.hpp"
#include "map/movemine.hpp"
#include "map/movespot.hpp"
#include "map/movesup.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/support.hpp"
#include "map/toward.hpp"
#include "types.hpp"
#include "units/orders.hpp"
#include "units/stats.hpp"
#include "units/unitname.hpp"
#include "units/value.hpp"

#include "glue.hpp"
#include "port.hpp"

using namespace opeu::game;

namespace {
constexpr uint16_t PATH_SEGMENT{ 0x4582 }; // the far path-cost grid and queue
constexpr uint8_t  DIRECTIONS_AND_CENTER{ 9 };
// Below opeu_move_adjacent's frame: the step coordinates.
constexpr uint16_t STEP_X_AT{ 2 };
constexpr uint16_t STEP_Y_AT{ 4 };
// Below opeu_move_unit_target's frame: the report and the unit name.
constexpr uint16_t REPORT_AT{ 0x68 };
constexpr uint16_t NAME_AT{ 0x74 };
// The message numbers naming the side that lost sight.
constexpr uint16_t SIDE_1_LOST{ 53 };
constexpr uint16_t SIDE_0_LOST{ 54 };
// Order progress: 0..99 counts down toward 0 (done), 100..199 counts up toward 200 (done), 255 not started.
constexpr uint8_t PROGRESS_UP{ 100 };
constexpr uint8_t PROGRESS_DONE_UP{ 200 };
constexpr uint8_t PROGRESS_UNSET{ 255 };
} // namespace

// Whether (x, y) is the current unit's cell or one next to it. frame: this routine's frame.
uint16_t opeu_move_adjacent(uint8_t* ds, HostEvent event, uint16_t frame, uint8_t x, uint8_t y) {
  auto const& data{ Data(ds) };
  auto const& unit{ Record<Unit>(data, data.current_unit_ptr) };
  auto const  x_at{ Local(frame, STEP_X_AT) };
  auto const  y_at{ Local(frame, STEP_Y_AT) };
  for (uint8_t direction{ 0 }; direction < DIRECTIONS_AND_CENTER; direction++) {
    Byte(ds, x_at) = unit.x;
    Byte(ds, y_at) = unit.y;
    if (Call(ds, event, MoveService::GRID_STEP, x_at, y_at, direction) && x == Byte(ds, x_at) && y == Byte(ds, y_at)) return 1;
  }
  return 0;
}

// The objective at the current unit's order destination when the side holds it; otherwise the order is restored and
// 0 returned. x and y: DS offsets of the bytes that receive the destination.
uint16_t opeu_move_objective_target(uint8_t* ds, HostEvent event, uint16_t x, uint16_t y, uint8_t side) {
  auto const current{ Data(ds).current_unit_ptr };
  if (Call(ds, event, MoveService::DESTINATION, current, x, y)) {
    auto const objective{ Call(ds, event, MoveService::FIND_OBJECTIVE, Byte(ds, x), Byte(ds, y)) };
    if (static_cast<uint8_t>(Call(ds, event, MoveService::OBJECTIVE_SIDE, objective)) == side) return objective;
  }
  Call(ds, event, MoveService::RESTORE_ORDER, current);
  return 0;
}

// The unit at the current unit's order destination. A player's side (or a computer side below delivery level 2) must
// still see it: a friendly unit must be selectable, an enemy one spotted; otherwise the loss of sight is reported, the
// order restored and 0 returned. frame: this routine's frame.
uint16_t opeu_move_unit_target(uint8_t* ds, HostEvent event, uint16_t frame, uint16_t x, uint16_t y) {
  auto const& data{ Data(ds) };
  auto const  current{ data.current_unit_ptr };
  if (Call(ds, event, MoveService::DESTINATION, current, x, y)) {
    auto const target{ Call(ds, event, MoveService::FIND_UNIT, Byte(ds, x), Byte(ds, y)) };
    if (data.side_control[data.current_side] != SIDE_PLAYED && data.delivery_level >= 2) return target;
    auto const side{ static_cast<uint8_t>(Call(ds, event, MoveService::UNIT_SIDE, target)) };
    auto const seen{ side == data.current_side ? Call(ds, event, MoveService::UNIT_ELIGIBLE, target) : Call(ds, event, MoveService::UNIT_SPOTTED, target) };
    if (seen) return target;
    auto const report{ Local(frame, REPORT_AT) };
    Call(ds, event, MoveService::FORMAT, report, Near(data, data.lost_sight_format), side == 0 ? SIDE_0_LOST : SIDE_1_LOST, data.side_names[side],
         Call(ds, event, MoveService::UNIT_NAME, target, Local(frame, NAME_AT)));
    Call(ds, event, MoveService::UNIT_MESSAGE, current, report);
  }
  Call(ds, event, MoveService::RESTORE_ORDER, current);
  return 0;
}

// Advances an upward order progress (an unset one starts at 200 - value); returns 1 when it reaches 200.
uint16_t opeu_move_level_raise(uint8_t* ds, uint16_t record, uint8_t value) {
  auto& progress{ Record<Order>(Data(ds), record).progress };
  if (progress >= PROGRESS_UP && progress < PROGRESS_DONE_UP) ++progress;
  if (progress == PROGRESS_UNSET) progress = static_cast<uint8_t>(PROGRESS_DONE_UP - value);
  return progress == PROGRESS_DONE_UP;
}

// Advances a downward order progress (a finished or unset one starts at value); returns 1 when it reaches 0.
uint16_t opeu_move_level_lower(uint8_t* ds, uint16_t record, uint8_t value) {
  auto& progress{ Record<Order>(Data(ds), record).progress };
  if (progress && progress < PROGRESS_UP) --progress;
  if (progress == PROGRESS_DONE_UP || progress == PROGRESS_UNSET) progress = value;
  return progress == 0;
}

uint16_t svc_map_move(uint8_t* ds, const char* module, uint16_t id, const uint16_t* args, uint16_t count) {
  auto const arg{ [&](unsigned i) { return Arg(args, count, i); } };
  auto const byte{ [&](unsigned i) { return static_cast<uint8_t>(Arg(args, count, i)); } };
  auto const in_map_frame{ [](auto&& body) { return WithMapFrame([&](uint16_t frame) { return body(frame); }); } };
  switch (static_cast<MoveService>(id)) {
  case MoveService::RATING:           return opeu_move_rating(ds, svc_movemine);
  case MoveService::GRID_STEP:        return opeu_grid_step(PORT_DS(arg(0)), PORT_DS(arg(1)), byte(2));
  case MoveService::COST:             return opeu_move_cost(ds, byte(0), byte(1), arg(2));
  case MoveService::DISTANCE:         return opeu_grid_distance(byte(0), byte(1), byte(2), byte(3));
  case MoveService::DESTINATION:      return opeu_unit_order_destination(ds, arg(0), PORT_DS(arg(1)), PORT_DS(arg(2)));
  case MoveService::REACHABLE:        return opeu_move_reachable(ds, reinterpret_cast<uint16_t*>(port_far(PATH_SEGMENT, 0)), byte(0), byte(1), byte(2), byte(3), arg(4));
  case MoveService::IS_ENGINEER:      return opeu_category_6(byte(0));
  case MoveService::STEP_TOWARD:      return opeu_grid_step_toward(ds, port_far(PATH_SEGMENT, 0), arg(0), arg(1), byte(2), byte(3));
  case MoveService::CLEAR_PATH:       opeu_move_clear(port_far(PATH_SEGMENT, 0), byte(0), byte(1)); return 0;
  case MoveService::FIND_OBJECTIVE:   return opeu_find_objective_at(ds, byte(0), byte(1));
  case MoveService::OBJECTIVE_SIDE:   return opeu_objective_side(ds, arg(0));
  case MoveService::FIND_UNIT:        return opeu_find_unit_at(ds, byte(0), byte(1));
  case MoveService::UNIT_SIDE:        return opeu_unit_side(arg(0));
  case MoveService::CELL_SIDE:        return opeu_cell_side(ds, byte(0), byte(1));
  case MoveService::SUPPLIED:         return opeu_move_supplied(ds);
  case MoveService::ARRIVAL_LOCATION: in_map_frame([&](uint16_t frame) { opeu_arrival_location(ds, svc_mappos, frame, byte(0), byte(1), byte(2)); }); return 0;
  case MoveService::ARRIVAL_VIEW:     in_map_frame([&](uint16_t frame) { opeu_arrival_view(ds, svc_mapunit, frame, arg(0)); }); return 0;
  case MoveService::UNIT_ERASE:       in_map_frame([&](uint16_t frame) { opeu_unit_remove_draw(ds, svc_maperase, frame, arg(0)); }); return 0;
  case MoveService::ARRIVAL_PREPARE:  in_map_frame([&](uint16_t frame) { opeu_arrival_prepare(ds, svc_maperase, frame, arg(0)); }); return 0;
  case MoveService::CLAMP:            opeu_move_clamp(ds); return 0;
  case MoveService::CELL_MARKED:      return opeu_cell_marked(ds, byte(0), byte(1), byte(2));
  case MoveService::MINES:            opeu_move_mines(ds, svc_movemine, byte(0), byte(1)); return 0;
  case MoveService::SPOT:             opeu_move_spot(ds, svc_movespot); return 0;
  case MoveService::CONSUME:          opeu_move_consume(ds); return 0;
  case MoveService::ENEMY_ADJACENT:   return in_map_frame([&](uint16_t frame) { return opeu_move_enemy_adjacent(ds, svc_movespot, frame); });
  case MoveService::RESTORE_ORDER:    opeu_restore_unit_order(ds, arg(0)); return 0;
  case MoveService::UNIT_ELIGIBLE:    return opeu_unit_selection_eligible(ds, arg(0));
  case MoveService::UNIT_SPOTTED:     return opeu_unit_special(ds, arg(0));
  case MoveService::UNIT_NAME:        return opeu_unit_name(ds, svc_unitname, arg(0), arg(1));
  case MoveService::FORMAT:           map_request_format(ds, args, count); return 0;
  case MoveService::UNIT_MESSAGE:     opeu_unit_report(ds, static_cast<DialogHost const*>(port_dialogs_host()), arg(0), arg(1)); return 0;
  default:                            port_unhandled(module, id, count);
  }
  return 0;
}

uint16_t svc_movetgt(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_map_move(ds, "movetgt", id, args, count);
}
