// The engineering orders: laying and clearing mines, building a bridge and repairing the arms of an objective or a
// regiment. Each handler takes the current regiment's order destination and needs the regiment next to it; mines and
// bridges lower the order's progress by a skill roll (the general's leadership, the regiment's averages, its engineers
// and a random term) and, when the progress runs out, report, change the map cell and show the day's picture whose
// finish routine ends the pending wait. A repair moves damaged arms back into service, up to the type's capacity, with
// engineers' strength / 10 + 1 per turn.
#include "units/engineer.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/order-service.hpp"
#include "core/limits.hpp"
#include "core/textout.hpp"
#include "types.hpp"
#include "units/order-handler.hpp"

#include "common/game.hpp"
#include "core/dialogs.hpp"
#include "core/objrank.hpp"
#include "core/recindex.hpp"
#include "core/sideutil.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "map/cellflag.hpp"
#include "map/grid.hpp"
#include "map/maperase.hpp"
#include "map/movesup.hpp"
#include "map/movetgt.hpp"
#include "map/moveunit.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/support.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/orders.hpp"
#include "units/stats.hpp"
#include "units/unitname.hpp"
#include "units/unitpers.hpp"
#include "units/value.hpp"

#include <utility>

using namespace opeu::game;

namespace {
// The calls the engineering handlers make through their event (served by svc_engineer).
enum Service : std::uint16_t {
  UNIT_PERSON           = 0,  // (unit) -> the regiment's general
  RAND_BOUNDED          = 1,  // (bound)
  UNIT_QUANTITY         = 2,  // (unit, kind, &count) -> strength of the regiment's elements of the kind
  UNIT_AVERAGE10        = 3,  // (unit)
  UNIT_AVERAGE11        = 4,  // (unit)
  ORDER_DESTINATION     = 5,  // (unit, &x, &y)
  CELL_MARKED           = 6,  // (x, y, side) -> nonzero when the side mined the cell
  MOVE_ADJACENT         = 7,  // (x, y) -> nonzero when the regiment is next to the cell
  LEVEL_LOWER           = 8,  // (order, amount) -> nonzero when the progress ran out
  CELL_SIDE             = 9,  // (x, y) -> the side holding the cell, 0xff for none
  UNIT_REPORT           = 10, // (unit, text)
  MARK_CELL             = 11, // (x, y, side): mine the cell
  MAP_ERASE             = 12, // (x, y)
  DAILY_SHOW            = 13, // (picture, side, finish offset, finish segment)
  RESTORE_ORDER         = 14, // (unit)
  UNMARK_CELL           = 15, // (x, y, side)
  MAP_CODE              = 16, // (x, y) -> terrain code
  MAP_MARK_BRIDGE       = 17, // (x, y)
  MOVE_OBJECTIVE_TARGET = 18, // (&x, &y, side) -> the objective of the order
  MOVE_UNIT_TARGET      = 19, // (&x, &y) -> the regiment of the order
  PENDING_FINISH        = 20, // (value)
  SERVICES              = 21,
};

// The pictures of the day's show and the link-table addresses of the routines that end it.
constexpr std::uint16_t ENGINEER_SEGMENT{ 0x319e };
enum Picture : std::uint16_t {
  MINES_LAID_PICTURE    = 0x191,
  MINES_REMOVED_PICTURE = 0x68,
  BRIDGE_PICTURE        = 0x1a2,
  ARMS_PICTURE          = 0x1b4,
};
enum Finish : std::uint16_t {
  MINES_LAID_FINISH    = 0x00a0,
  MINES_REMOVED_FINISH = 0x016a,
  BRIDGE_BUILT_FINISH  = 0x02b8,
  ARMS_REPAIRED_FINISH = 0x03ba,
};

// The engineering skill roll, computed in 16-bit words of which the low byte is kept. The services are called in the
// order general, random bound, engineer count, average10, average11.
auto SkillRoll(Game& game, HostEvent event, std::uint16_t person, std::uint16_t bound, std::uint16_t leadership_divisor, std::uint16_t engineer_divisor, std::uint16_t bonus) -> std::uint8_t {
  auto&               data{ Data(game) };
  std::uint16_t const random{ Call(game, event, RAND_BOUNDED, bound) };
  ServiceLocals const count{ 2 };
  std::uint16_t const engineers{ Call(game, event, UNIT_QUANTITY, data.current_unit_ptr, Bits(Category::ENGINEERS), count.At(0)) };
  std::uint16_t const average10{ Call(game, event, UNIT_AVERAGE10, data.current_unit_ptr) };
  std::uint16_t const average11{ Call(game, event, UNIT_AVERAGE11, data.current_unit_ptr) };
  return static_cast<std::uint8_t>(average11 / 19 - static_cast<std::uint16_t>(Record<Person>(data, person).leadership + average10) / leadership_divisor - engineers / engineer_divisor + random
                                   + bonus);
}

// Lowers the current order's progress by the roll; nonzero when it runs out. The regiment is read after the roll.
auto LowerProgress(Game& game, HostEvent event, std::uint8_t roll) -> std::uint16_t {
  auto& data{ Data(game) };
  return Call(game, event, LEVEL_LOWER, Near(data, Record<Unit>(data, data.current_unit_ptr).orders[0]), roll);
}

auto Show(Game& game, HostEvent event, Picture picture, Finish finish) -> void {
  Call(game, event, DAILY_SHOW, picture, Data(game).current_side, finish, ENGINEER_SEGMENT);
}

auto Report(Game& game, HostEvent event, auto const& text) -> void {
  auto& data{ Data(game) };
  Call(game, event, UNIT_REPORT, data.current_unit_ptr, Near(data, text));
}

auto RestoreOrder(Game& game, HostEvent event) -> void {
  Call(game, event, RESTORE_ORDER, Data(game).current_unit_ptr);
}
} // namespace

uint8_t opeu_mine_skill(Game& game, HostEvent event) {
  auto const person{ Call(game, event, UNIT_PERSON, Data(game).current_unit_ptr) };
  return SkillRoll(game, event, person, RollBound(Data(game)), 45, 96, 8);
}

void opeu_mines_laid_finish(Game& game, HostEvent event) {
  Call(game, event, PENDING_FINISH, SHOW_DONE);
}

// Lays mines at the destination cell unless the side has mined it already or the regiment stands on it.
uint16_t opeu_engineer_lay_mines(Game& game, HostEvent event) {
  auto&             data{ Data(game) };
  ServiceCell const destination;
  auto const        at_x{ destination.XAt() };
  auto const        at_y{ destination.YAt() };
  Call(game, event, ORDER_DESTINATION, data.current_unit_ptr, at_x, at_y);
  auto const  x{ Byte(game, at_x) };
  auto const  y{ Byte(game, at_y) };
  auto const& regiment{ Record<Unit>(data, data.current_unit_ptr) };
  if (!Call(game, event, CELL_MARKED, x, y, data.current_side) && (regiment.x != x || regiment.y != y)) {
    if (!Call(game, event, MOVE_ADJACENT, x, y)) return 0;
    if (LowerProgress(game, event, opeu_mine_skill(game, event)) && static_cast<std::uint8_t>(Call(game, event, CELL_SIDE, x, y)) == NO_SIDE) {
      Report(game, event, data.mines_laid_text);
      Call(game, event, MARK_CELL, x, y, data.current_side);
      Call(game, event, MAP_ERASE, x, y);
      Show(game, event, MINES_LAID_PICTURE, MINES_LAID_FINISH);
      RestoreOrder(game, event);
    }
    return 1;
  }
  RestoreOrder(game, event);
  return 1;
}

void opeu_mines_removed_finish(Game& game, HostEvent event) {
  Call(game, event, PENDING_FINISH, SHOW_DONE);
}

// Clears the mines of both sides at the destination cell.
uint16_t opeu_engineer_remove_mines(Game& game, HostEvent event) {
  auto&             data{ Data(game) };
  ServiceCell const destination;
  auto const        at_x{ destination.XAt() };
  auto const        at_y{ destination.YAt() };
  Call(game, event, ORDER_DESTINATION, data.current_unit_ptr, at_x, at_y);
  auto const x{ Byte(game, at_x) };
  auto const y{ Byte(game, at_y) };
  if (Call(game, event, CELL_MARKED, x, y, data.current_side)) {
    if (!Call(game, event, MOVE_ADJACENT, x, y)) return 0;
    if (LowerProgress(game, event, opeu_mine_skill(game, event))) {
      Report(game, event, data.mines_removed_text);
      Call(game, event, UNMARK_CELL, x, y, 0);
      Call(game, event, UNMARK_CELL, x, y, 1);
      Call(game, event, MAP_ERASE, x, y);
      Show(game, event, MINES_REMOVED_PICTURE, MINES_REMOVED_FINISH);
      RestoreOrder(game, event);
    }
    return 1;
  }
  RestoreOrder(game, event);
  return 1;
}

uint8_t opeu_bridge_skill(Game& game, HostEvent event) {
  auto const person{ Call(game, event, UNIT_PERSON, Data(game).current_unit_ptr) };
  return SkillRoll(game, event, person, RollBound(Data(game)), 35, 80, 10);
}

void opeu_bridge_built_finish(Game& game, HostEvent event) {
  Call(game, event, PENDING_FINISH, SHOW_DONE);
}

// Builds a bridge over the river at the destination cell.
uint16_t opeu_engineer_build_bridge(Game& game, HostEvent event) {
  auto&             data{ Data(game) };
  ServiceCell const destination;
  auto const        at_x{ destination.XAt() };
  auto const        at_y{ destination.YAt() };
  Call(game, event, ORDER_DESTINATION, data.current_unit_ptr, at_x, at_y);
  auto const x{ Byte(game, at_x) };
  auto const y{ Byte(game, at_y) };
  if (static_cast<Terrain>(static_cast<std::uint8_t>(Call(game, event, MAP_CODE, x, y))) == Terrain::RIVER) {
    if (!Call(game, event, MOVE_ADJACENT, x, y)) return 0;
    if (LowerProgress(game, event, opeu_bridge_skill(game, event)) && static_cast<std::uint8_t>(Call(game, event, CELL_SIDE, x, y)) == NO_SIDE) {
      Report(game, event, data.bridge_built_text);
      Call(game, event, MAP_MARK_BRIDGE, x, y);
      Call(game, event, MAP_ERASE, x, y);
      Show(game, event, BRIDGE_PICTURE, BRIDGE_BUILT_FINISH);
      RestoreOrder(game, event);
    }
    return 1;
  }
  RestoreOrder(game, event);
  return 1;
}

// Moves damaged arms of the item (an objective's item or an element's type, quantity and damage) back into service:
// at most the budget (lowered by what is repaired), the damaged count, and what the type's capacity leaves room for. Nonzero when the
// item had damage and budget left.
uint16_t opeu_arms_repair(Game& game, uint16_t& budget, uint16_t body) {
  auto& data{ Data(game) };
  auto& item{ Record<Item>(data, body) };
  if (!budget || !item.extra) return 0;
  auto const capacity{ data.request_capacities[data.element_types[item.type].category] };
  auto const repaired{ static_cast<std::uint16_t>(opeu_min16(opeu_min16(budget, item.extra) + item.quantity, capacity) - item.quantity) };
  item.extra    -= repaired;
  item.quantity += repaired;
  budget         = static_cast<std::uint16_t>(budget - repaired);
  return 1;
}

void opeu_arms_repaired_finish(Game& game, HostEvent event) {
  Call(game, event, PENDING_FINISH, SHOW_DONE);
}

// Repairs the arms of the own objective at the destination; the order ends when nothing is left to repair.
uint16_t opeu_engineer_repair_objective(Game& game, HostEvent event) {
  auto&               data{ Data(game) };
  ServiceLocals const counts{ 2 };
  auto const          crews{ counts.At(0) };
  if (!Call(game, event, UNIT_QUANTITY, data.current_unit_ptr, Bits(Category::REPAIR), crews)) {
    RestoreOrder(game, event);
    return 1;
  }
  ServiceCell const target;
  auto const        at_x{ target.XAt() };
  auto const        at_y{ target.YAt() };
  auto const        objective{ Call(game, event, MOVE_OBJECTIVE_TARGET, at_x, at_y, data.current_side) };
  if (!objective) return 1;
  if (!Call(game, event, MOVE_ADJACENT, Byte(game, at_x), Byte(game, at_y))) return 0;
  Record<Unit>(data, data.current_unit_ptr).orders[0].progress = 0;
  auto budget{ static_cast<std::uint16_t>(Call(game, event, UNIT_QUANTITY, data.current_unit_ptr, Bits(Category::REPAIR), crews) / 10 + 1) };
  bool repaired{ false };
  for (auto& item : Record<Objective>(data, objective).items)
    if (item.quantity && opeu_arms_repair(game, budget, Near(data, item))) repaired = true;
  if (!repaired) {
    Report(game, event, data.arms_repaired_text);
    Show(game, event, ARMS_PICTURE, ARMS_REPAIRED_FINISH);
    RestoreOrder(game, event);
  }
  return 1;
}

// Repairs the arms of the friendly regiment the order targets; the order ends when nothing is left to repair.
uint16_t opeu_engineer_repair_unit(Game& game, HostEvent event) {
  auto&               data{ Data(game) };
  ServiceLocals const counts{ 2 };
  auto const          crews{ counts.At(0) };
  if (!Call(game, event, UNIT_QUANTITY, data.current_unit_ptr, Bits(Category::REPAIR), crews)) {
    RestoreOrder(game, event);
    return 1;
  }
  ServiceCell const target;
  auto const        at_x{ target.XAt() };
  auto const        at_y{ target.YAt() };
  auto const        unit{ Call(game, event, MOVE_UNIT_TARGET, at_x, at_y) };
  if (!unit) return 1;
  if (!Call(game, event, MOVE_ADJACENT, Byte(game, at_x), Byte(game, at_y))) return 0;
  Record<Unit>(data, data.current_unit_ptr).orders[0].progress = 0;
  auto budget{ static_cast<std::uint16_t>(Call(game, event, UNIT_QUANTITY, data.current_unit_ptr, Bits(Category::REPAIR), crews) / 10 + 1) };
  bool repaired{ false };
  for (auto element{ Record<Unit>(data, unit).first }; element; element = Record<Element>(data, element).next)
    if (opeu_arms_repair(game, budget, Near(data, Record<Element>(data, element).type))) repaired = true;
  if (!repaired) {
    Report(game, event, data.unit_arms_repaired_text);
    Show(game, event, ARMS_PICTURE, ARMS_REPAIRED_FINISH);
    RestoreOrder(game, event);
  }
  return 1;
}

// The routines behind the engineering handlers' calls. The calls they share with the other order handlers go
// through svc_order_handler.
uint16_t svc_engineer(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  // The order handler service of each engineering call; NONE for the calls served here.
  constexpr auto                                      NONE{ static_cast<OrderService>(0xff) };
  static constexpr std::array<OrderService, SERVICES> shared{
    OrderService::UNIT_PERSON,
    OrderService::RAND_BOUNDED,
    NONE,
    OrderService::UNIT_AVERAGE10,
    OrderService::UNIT_AVERAGE11,
    OrderService::UNIT_ORDER_DESTINATION,
    NONE,
    OrderService::MOVE_ADJACENT,
    OrderService::MOVE_LEVEL_LOWER,
    OrderService::CELL_SIDE,
    OrderService::UNIT_REPORT,
    NONE,
    OrderService::MAP_ERASE,
    OrderService::DAILY_SHOW,
    OrderService::RESTORE_UNIT_ORDER,
    NONE,
    OrderService::MAP_CODE,
    NONE,
    OrderService::MOVE_OBJECTIVE_TARGET,
    OrderService::MOVE_UNIT_TARGET,
    OrderService::PENDING_FINISH,
  };
  uint16_t r = 0;
  switch (id) {
  case UNIT_QUANTITY:   r = opeu_unit_quantity(game, Arg(args, count, 0), (uint8_t)Arg(args, count, 1), game.State().Bytes(Arg(args, count, 2), 1).data()); break;
  case CELL_MARKED:     r = opeu_cell_marked(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case MARK_CELL:       opeu_mark_cell(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case UNMARK_CELL:     opeu_unmark_cell(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case MAP_MARK_BRIDGE: opeu_map_mark_10(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  default:
    if (id >= SERVICES || shared[id] == NONE || !svc_order_handler(game, shared[id], args, count, &r)) port_unhandled("engineer", id, count);
  }
  return r;
}

// The services the order handlers share (OrderService); 0 when the id is not one of them. Pointer arguments are DS
// offsets; byte arguments narrow.
int svc_order_handler(Game& game, uint16_t id, const uint16_t* args, uint16_t count, uint16_t* result) {
  uint16_t r = 0;
  switch (id) {
  case OrderService::UNIT_AVERAGE11:     r = opeu_unit_average11(game, Arg(args, count, 0)); break;
  case OrderService::RAND_BOUNDED:       r = port_rand_bounded(game, Arg(args, count, 0)); break;
  case OrderService::RESTORE_UNIT_ORDER: opeu_restore_unit_order(game, Arg(args, count, 0)); break;
  case OrderService::GRID_STEP:          r = opeu_grid_step(game.State().Bytes(Arg(args, count, 0), 1).data(), game.State().Bytes(Arg(args, count, 1), 1).data(), (uint8_t)Arg(args, count, 2)); break;
  case OrderService::FIND_UNIT_AT:       r = opeu_find_unit_at(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case OrderService::UNIT_SIDE:          r = opeu_unit_side(Arg(args, count, 0)); break;
  case OrderService::UNIT_VALUE:         r = opeu_unit_value(game, Arg(args, count, 0)); break;
  case OrderService::MOVE_SUPPLIED:      r = opeu_move_supplied(game); break;
  case OrderService::UNIT_REPORT:        opeu_unit_report(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case OrderService::UNIT_SELECTION_ELIGIBLE: r = opeu_unit_selection_eligible(game, Arg(args, count, 0)); break;
  case OrderService::REQUEST_FORMAT:          opeu_request_format(game, { args, count }); break;
  case OrderService::CONFIRM_MESSAGE:         r = opeu_confirm_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case OrderService::DAILY_SHOW:              opeu_daily_show(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), (uint8_t)Arg(args, count, 1), Arg32(args, count, 2)); break;
  case OrderService::OPPOSITE_SIDE:           r = opeu_opposite_side((uint8_t)Arg(args, count, 0)); break;
  case OrderService::MOVE_OBJECTIVE_TARGET:   r = opeu_move_objective_target(game, Arg(args, count, 0), Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case OrderService::MOVE_ADJACENT:           r = opeu_move_adjacent(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case OrderService::MOVE_LEVEL_LOWER:        r = opeu_move_level_lower(game, Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case OrderService::CELL_SIDE:               r = opeu_cell_side(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case OrderService::OBJECTIVE_QUANTITY:      r = opeu_objective_quantity(game, Arg(args, count, 0), (uint8_t)Arg(args, count, 1), game.State().Bytes(Arg(args, count, 2), 1).data()); break;
  case OrderService::MOVE_UNIT:               opeu_move_unit(game, Arg(args, count, 0)); break;
  case OrderService::SIDE_GROUPS_RETARGET:    opeu_side_groups_retarget(game, (uint8_t)Arg(args, count, 0)); break;
  case OrderService::MOVE_UNIT_TARGET:        r = opeu_move_unit_target(game, Arg(args, count, 0), Arg(args, count, 1)); break;
  case OrderService::UNIT_NAME:               r = opeu_unit_name(game, svc_unitname, Arg(args, count, 0), Arg(args, count, 1)); break;
  case OrderService::ABS16_BITS:              r = opeu_abs16_bits(Arg(args, count, 0)); break;
  case OrderService::UNIT_PERSON:             r = opeu_unit_person(game, Arg(args, count, 0)); break;
  case OrderService::UNIT_AVERAGE10:          r = opeu_unit_average10(game, Arg(args, count, 0)); break;
  case OrderService::UNIT_AVERAGE9:           r = opeu_unit_average9(game, Arg(args, count, 0)); break;
  case OrderService::UNIT_ORDER_DESTINATION:
    r = opeu_unit_order_destination(game, Arg(args, count, 0), game.State().Bytes(Arg(args, count, 1), 1).data(), game.State().Bytes(Arg(args, count, 2), 1).data());
    break;
  case OrderService::MAP_CODE:       r = opeu_map_code(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case OrderService::MAP_MARK5:      opeu_map_mark_5(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case OrderService::MAP_ERASE:      opeu_map_erase(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case OrderService::PENDING_FINISH: opeu_pending_finish(game, (uint8_t)Arg(args, count, 0)); break;
  case OrderService::WAIT_UNITS:     opeu_delay_units(game, (uint8_t)Arg(args, count, 0)); break;
  case OrderService::SOUND_CUE5:     opeu_sound_cue5(game, svc_soundcue); break;
  default:                           return 0;
  }
  *result = r;
  return 1;
}

auto port_routine_opeu_arms_repaired_finish(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_arms_repaired_finish(game, svc_engineer);
  return 0;
}

auto port_routine_opeu_bridge_built_finish(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_bridge_built_finish(game, svc_engineer);
  return 0;
}

auto port_routine_opeu_engineer_build_bridge(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_engineer_build_bridge(game, svc_engineer);
}

auto port_routine_opeu_engineer_lay_mines(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_engineer_lay_mines(game, svc_engineer);
}

auto port_routine_opeu_engineer_remove_mines(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_engineer_remove_mines(game, svc_engineer);
}

auto port_routine_opeu_engineer_repair_objective(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_engineer_repair_objective(game, svc_engineer);
}

auto port_routine_opeu_engineer_repair_unit(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_engineer_repair_unit(game, svc_engineer);
}

auto port_routine_opeu_mines_laid_finish(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_mines_laid_finish(game, svc_engineer);
  return 0;
}

auto port_routine_opeu_mines_removed_finish(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_mines_removed_finish(game, svc_engineer);
  return 0;
}
