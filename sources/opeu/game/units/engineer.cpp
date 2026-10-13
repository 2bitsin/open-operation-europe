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
auto SkillRoll(Game& game, std::uint16_t person, std::uint16_t bound, std::uint16_t leadership_divisor, std::uint16_t engineer_divisor, std::uint16_t bonus) -> std::uint8_t {
  auto&               data{ Data(game) };
  std::uint16_t const random{ port_rand_bounded(game, bound) };
  ServiceLocals const count{ 2 };
  auto const          hoisted0{ static_cast<std::uint16_t>(data.current_unit_ptr) };
  auto const          hoisted1{ static_cast<std::uint16_t>(Bits(Category::ENGINEERS)) };
  auto const          hoisted2{ static_cast<std::uint16_t>(count.At(0)) };
  std::uint16_t const engineers{ opeu_unit_quantity(game, hoisted0, static_cast<uint8_t>(hoisted1), game.State().Bytes(hoisted2, 1).data()) };
  std::uint16_t const average10{ opeu_unit_average10(game, data.current_unit_ptr) };
  std::uint16_t const average11{ opeu_unit_average11(game, data.current_unit_ptr) };
  return static_cast<std::uint8_t>(average11 / 19 - static_cast<std::uint16_t>(Record<Person>(data, person).leadership + average10) / leadership_divisor - engineers / engineer_divisor + random
                                   + bonus);
}

// Lowers the current order's progress by the roll; nonzero when it runs out. The regiment is read after the roll.
auto LowerProgress(Game& game, std::uint8_t roll) -> std::uint16_t {
  auto& data{ Data(game) };
  return opeu_move_level_lower(game, Near(data, Record<Unit>(data, data.current_unit_ptr).orders[0]), static_cast<uint8_t>(roll));
}

auto Show(Game& game, Picture picture, Finish finish) -> void {
  opeu_daily_show(game, static_cast<const DialogHost*>(port_dialogs_host()), picture, static_cast<uint8_t>(Data(game).current_side),
                  (static_cast<std::uint32_t>(static_cast<std::uint16_t>(finish)) | static_cast<std::uint32_t>(static_cast<std::uint16_t>(ENGINEER_SEGMENT)) << 16));
}

auto Report(Game& game, auto const& text) -> void {
  auto& data{ Data(game) };
  opeu_unit_report(game, static_cast<const DialogHost*>(port_dialogs_host()), data.current_unit_ptr, Near(data, text));
}

auto RestoreOrder(Game& game) -> void {
  opeu_restore_unit_order(game, Data(game).current_unit_ptr);
}
} // namespace

uint8_t opeu_mine_skill(Game& game) {
  auto const person{ opeu_unit_person(game, Data(game).current_unit_ptr) };
  return SkillRoll(game, person, RollBound(Data(game)), 45, 96, 8);
}

void opeu_mines_laid_finish(Game& game) {
  opeu_pending_finish(game, static_cast<uint8_t>(SHOW_DONE));
}

// Lays mines at the destination cell unless the side has mined it already or the regiment stands on it.
uint16_t opeu_engineer_lay_mines(Game& game) {
  auto&             data{ Data(game) };
  ServiceCell const destination;
  auto const        at_x{ destination.XAt() };
  auto const        at_y{ destination.YAt() };
  opeu_unit_order_destination(game, data.current_unit_ptr, game.State().Bytes(at_x, 1).data(), game.State().Bytes(at_y, 1).data());
  auto const  x{ Byte(game, at_x) };
  auto const  y{ Byte(game, at_y) };
  auto const& regiment{ Record<Unit>(data, data.current_unit_ptr) };
  if (!opeu_cell_marked(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y), static_cast<uint8_t>(data.current_side)) && (regiment.x != x || regiment.y != y)) {
    if (!opeu_move_adjacent(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y))) return 0;
    if (LowerProgress(game, opeu_mine_skill(game)) && static_cast<std::uint8_t>(opeu_cell_side(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y))) == NO_SIDE) {
      Report(game, data.mines_laid_text);
      opeu_mark_cell(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y), static_cast<uint8_t>(data.current_side));
      opeu_map_erase(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y));
      Show(game, MINES_LAID_PICTURE, MINES_LAID_FINISH);
      RestoreOrder(game);
    }
    return 1;
  }
  RestoreOrder(game);
  return 1;
}

void opeu_mines_removed_finish(Game& game) {
  opeu_pending_finish(game, static_cast<uint8_t>(SHOW_DONE));
}

// Clears the mines of both sides at the destination cell.
uint16_t opeu_engineer_remove_mines(Game& game) {
  auto&             data{ Data(game) };
  ServiceCell const destination;
  auto const        at_x{ destination.XAt() };
  auto const        at_y{ destination.YAt() };
  opeu_unit_order_destination(game, data.current_unit_ptr, game.State().Bytes(at_x, 1).data(), game.State().Bytes(at_y, 1).data());
  auto const x{ Byte(game, at_x) };
  auto const y{ Byte(game, at_y) };
  if (opeu_cell_marked(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y), static_cast<uint8_t>(data.current_side))) {
    if (!opeu_move_adjacent(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y))) return 0;
    if (LowerProgress(game, opeu_mine_skill(game))) {
      Report(game, data.mines_removed_text);
      opeu_unmark_cell(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y), static_cast<uint8_t>(0));
      opeu_unmark_cell(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y), static_cast<uint8_t>(1));
      opeu_map_erase(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y));
      Show(game, MINES_REMOVED_PICTURE, MINES_REMOVED_FINISH);
      RestoreOrder(game);
    }
    return 1;
  }
  RestoreOrder(game);
  return 1;
}

uint8_t opeu_bridge_skill(Game& game) {
  auto const person{ opeu_unit_person(game, Data(game).current_unit_ptr) };
  return SkillRoll(game, person, RollBound(Data(game)), 35, 80, 10);
}

void opeu_bridge_built_finish(Game& game) {
  opeu_pending_finish(game, static_cast<uint8_t>(SHOW_DONE));
}

// Builds a bridge over the river at the destination cell.
uint16_t opeu_engineer_build_bridge(Game& game) {
  auto&             data{ Data(game) };
  ServiceCell const destination;
  auto const        at_x{ destination.XAt() };
  auto const        at_y{ destination.YAt() };
  opeu_unit_order_destination(game, data.current_unit_ptr, game.State().Bytes(at_x, 1).data(), game.State().Bytes(at_y, 1).data());
  auto const x{ Byte(game, at_x) };
  auto const y{ Byte(game, at_y) };
  if (static_cast<Terrain>(static_cast<std::uint8_t>(opeu_map_code(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y)))) == Terrain::RIVER) {
    if (!opeu_move_adjacent(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y))) return 0;
    if (LowerProgress(game, opeu_bridge_skill(game)) && static_cast<std::uint8_t>(opeu_cell_side(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y))) == NO_SIDE) {
      Report(game, data.bridge_built_text);
      opeu_map_mark_10(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y));
      opeu_map_erase(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y));
      Show(game, BRIDGE_PICTURE, BRIDGE_BUILT_FINISH);
      RestoreOrder(game);
    }
    return 1;
  }
  RestoreOrder(game);
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

void opeu_arms_repaired_finish(Game& game) {
  opeu_pending_finish(game, static_cast<uint8_t>(SHOW_DONE));
}

// Repairs the arms of the own objective at the destination; the order ends when nothing is left to repair.
uint16_t opeu_engineer_repair_objective(Game& game) {
  auto&               data{ Data(game) };
  ServiceLocals const counts{ 2 };
  auto const          crews{ counts.At(0) };
  if (!opeu_unit_quantity(game, data.current_unit_ptr, static_cast<uint8_t>(Bits(Category::REPAIR)), game.State().Bytes(crews, 1).data())) {
    RestoreOrder(game);
    return 1;
  }
  ServiceCell const target;
  auto const        at_x{ target.XAt() };
  auto const        at_y{ target.YAt() };
  auto const        objective{ opeu_move_objective_target(game, at_x, at_y, static_cast<uint8_t>(data.current_side)) };
  if (!objective) return 1;
  if (!opeu_move_adjacent(game, static_cast<uint8_t>(Byte(game, at_x)), static_cast<uint8_t>(Byte(game, at_y)))) return 0;
  Record<Unit>(data, data.current_unit_ptr).orders[0].progress = 0;
  auto budget{ static_cast<std::uint16_t>(opeu_unit_quantity(game, data.current_unit_ptr, static_cast<uint8_t>(Bits(Category::REPAIR)), game.State().Bytes(crews, 1).data()) / 10 + 1) };
  bool repaired{ false };
  for (auto& item : Record<Objective>(data, objective).items)
    if (item.quantity && opeu_arms_repair(game, budget, Near(data, item))) repaired = true;
  if (!repaired) {
    Report(game, data.arms_repaired_text);
    Show(game, ARMS_PICTURE, ARMS_REPAIRED_FINISH);
    RestoreOrder(game);
  }
  return 1;
}

// Repairs the arms of the friendly regiment the order targets; the order ends when nothing is left to repair.
uint16_t opeu_engineer_repair_unit(Game& game) {
  auto&               data{ Data(game) };
  ServiceLocals const counts{ 2 };
  auto const          crews{ counts.At(0) };
  if (!opeu_unit_quantity(game, data.current_unit_ptr, static_cast<uint8_t>(Bits(Category::REPAIR)), game.State().Bytes(crews, 1).data())) {
    RestoreOrder(game);
    return 1;
  }
  ServiceCell const target;
  auto const        at_x{ target.XAt() };
  auto const        at_y{ target.YAt() };
  auto const        unit{ opeu_move_unit_target(game, at_x, at_y) };
  if (!unit) return 1;
  if (!opeu_move_adjacent(game, static_cast<uint8_t>(Byte(game, at_x)), static_cast<uint8_t>(Byte(game, at_y)))) return 0;
  Record<Unit>(data, data.current_unit_ptr).orders[0].progress = 0;
  auto budget{ static_cast<std::uint16_t>(opeu_unit_quantity(game, data.current_unit_ptr, static_cast<uint8_t>(Bits(Category::REPAIR)), game.State().Bytes(crews, 1).data()) / 10 + 1) };
  bool repaired{ false };
  for (auto element{ Record<Unit>(data, unit).first }; element; element = Record<Element>(data, element).next)
    if (opeu_arms_repair(game, budget, Near(data, Record<Element>(data, element).type))) repaired = true;
  if (!repaired) {
    Report(game, data.unit_arms_repaired_text);
    Show(game, ARMS_PICTURE, ARMS_REPAIRED_FINISH);
    RestoreOrder(game);
  }
  return 1;
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
  opeu_arms_repaired_finish(game);
  return 0;
}

auto port_routine_opeu_bridge_built_finish(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_bridge_built_finish(game);
  return 0;
}

auto port_routine_opeu_engineer_build_bridge(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_engineer_build_bridge(game);
}

auto port_routine_opeu_engineer_lay_mines(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_engineer_lay_mines(game);
}

auto port_routine_opeu_engineer_remove_mines(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_engineer_remove_mines(game);
}

auto port_routine_opeu_engineer_repair_objective(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_engineer_repair_objective(game);
}

auto port_routine_opeu_engineer_repair_unit(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_engineer_repair_unit(game);
}

auto port_routine_opeu_mines_laid_finish(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_mines_laid_finish(game);
  return 0;
}

auto port_routine_opeu_mines_removed_finish(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_mines_removed_finish(game);
  return 0;
}
