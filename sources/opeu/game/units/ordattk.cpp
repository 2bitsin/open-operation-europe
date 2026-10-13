// The occupy and attack orders. Next to its target the regiment lowers the order's progress by a loss roll; when it
// runs out, an empty enemy objective is taken, otherwise the regiment opens a battle (after the player confirms it for
// a player's division that the computer does not run). The locals the services write through their addresses stay in
// the data segment, each routine's in its own reserve.
#include "units/ordattk.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/order.hpp"
#include "types.hpp"
#include "units/order-handler.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/engineer.hpp"

using namespace opeu::game;

namespace {
// The locals the services read and write, as offsets below the top of the routine's reserve.
enum ServiceLocal : std::uint16_t {
  STEP_X       = 2, // attack_allowed: the neighbour cell
  STEP_Y       = 4,
  OCCUPY_X     = 6, // occupy: the objective's cell
  OCCUPY_Y     = 0x6c,
  OCCUPY_COUNT = 4,    // occupy: objective_quantity's count
  OCCUPY_TEXT  = 0x6a, // occupy: the report
  ATTACK_X     = 4,    // attack: the target regiment's cell
  ATTACK_Y     = 0x30,
  ATTACK_NAME  = 0x10, // attack: the target's name
  ATTACK_TEXT  = 0x2e, // attack: the target's label
  START_TEXT   = 0x66, // attack_start: the confirmation and report text
};
// The reserves: each routine's deepest local.
constexpr std::uint16_t ALLOWED_BYTES{ STEP_Y };
constexpr std::uint16_t START_BYTES{ START_TEXT };
constexpr std::uint16_t OCCUPY_BYTES{ OCCUPY_Y };
constexpr std::uint16_t ATTACK_BYTES{ ATTACK_Y };

// The pictures of the day's show and the link-table addresses of the routines that end it.
constexpr std::uint16_t ORDER_SEGMENT{ 0x3131 };
constexpr std::uint16_t BATTLE_PICTURE{ 0x1d7 };
constexpr std::uint16_t ATTACK_PRESENT{ 0x00f6 };
constexpr std::uint16_t OCCUPIED_PICTURE{ 0xa2 };
constexpr std::uint16_t OCCUPIED_PRESENT{ 0x0282 };
// objective_quantity's category for all the objective's arms.
constexpr std::uint16_t ALL_ARMS{ 0xff };
// The ticks wait_units waits after an occupation.
constexpr std::uint16_t OCCUPIED_WAIT{ 0x1e };
// The colour codes of the side names in a regiment label.
constexpr std::uint16_t SIDE0_COLOUR{ '5' };
constexpr std::uint16_t SIDE1_COLOUR{ '6' };

auto Random(std::uint8_t* ds, HostEvent event) -> std::uint16_t {
  return Call(ds, event, OrderService::RAND_BOUNDED, RollBound(Data(ds)));
}

auto Restore(std::uint8_t* ds, HostEvent event) -> void {
  Call(ds, event, OrderService::RESTORE_UNIT_ORDER, Data(ds).current_unit_ptr);
}

// Records the battle the current regiment opens against the cell x, y.
auto Engage(DataSegment& data, std::uint8_t x, std::uint8_t y) -> void {
  auto const& unit{ CurrentUnit(data) };
  data.battle_attacker_x          = unit.x;
  data.battle_attacker_y          = unit.y;
  data.battle_x                   = x;
  data.battle_y                   = y;
  data.turn_notification_bytes[0] = 1;
  data.turn_notification_bytes[1] = 0;
}

// The loss roll of occupy and attack: the general's leadership and bravery against the regiment's averages.
auto Loss(std::uint8_t* ds, HostEvent event, std::uint16_t divisor) -> std::uint8_t {
  auto&               data{ Data(ds) };
  auto const          person{ Call(ds, event, OrderService::UNIT_PERSON, data.current_unit_ptr) };
  std::uint16_t const random{ Random(ds, event) };
  std::uint16_t const average10{ Call(ds, event, OrderService::UNIT_AVERAGE10, data.current_unit_ptr) };
  std::uint16_t const average9{ Call(ds, event, OrderService::UNIT_AVERAGE9, data.current_unit_ptr) };
  std::uint16_t const average11{ Call(ds, event, OrderService::UNIT_AVERAGE11, data.current_unit_ptr) };
  auto const&         general{ Record<Person>(data, person) };
  return static_cast<std::uint8_t>(average11 / 19 - static_cast<std::uint16_t>(general.leadership + average10 + average9) / divisor - general.bravery / 80 + random + 6);
}
} // namespace

// Whether the current regiment may attack the cell x, y. A regiment whose average value11 reaches 70 + rand(30)
// gives up its order; no attack starts while a friendly neighbour's occupy or attack is under way, nor against a
// defending regiment worth more than three times as much.
uint16_t opeu_order_attack_allowed(uint8_t ds[65536], HostEvent event, uint8_t x, uint8_t y) {
  auto&      data{ Data(ds) };
  auto const average{ Call(ds, event, OrderService::UNIT_AVERAGE11, data.current_unit_ptr) };
  if (average >= static_cast<std::uint16_t>(Call(ds, event, OrderService::RAND_BOUNDED, 30) + 70)) {
    Restore(ds, event);
    return 0;
  }
  ServiceLocals const locals{ ALLOWED_BYTES };
  auto const          local{ [&](ServiceLocal offset) { return locals.At(static_cast<std::uint16_t>(ALLOWED_BYTES - offset)); } };
  auto const          step_x{ local(STEP_X) };
  auto const          step_y{ local(STEP_Y) };
  for (std::uint8_t direction{ 0 }; direction < 8; ++direction) {
    Byte(ds, step_x) = x;
    Byte(ds, step_y) = y;
    if (!Call(ds, event, OrderService::GRID_STEP, step_x, step_y, direction)) continue;
    auto const unit{ Call(ds, event, OrderService::FIND_UNIT_AT, Byte(ds, step_x), Byte(ds, step_y)) };
    if (!unit || static_cast<std::uint8_t>(Call(ds, event, OrderService::UNIT_SIDE, unit)) != data.current_side) continue;
    auto const& order{ Record<Unit>(data, unit).orders[0] };
    if ((order.kind == Kind(UnitOrder::OCCUPY) || order.kind == Kind(UnitOrder::ATTACK)) && order.progress) return 0;
  }
  auto const defender{ Call(ds, event, OrderService::FIND_UNIT_AT, x, y) };
  if (defender) {
    auto const& order{ Record<Unit>(data, defender).orders[0] };
    if (order.kind == Kind(UnitOrder::DEFEND) && order.progress == 0) {
      auto const own{ static_cast<std::uint16_t>(Call(ds, event, OrderService::UNIT_VALUE, data.current_unit_ptr) + 1) };
      auto const other{ static_cast<std::uint16_t>(Call(ds, event, OrderService::UNIT_VALUE, defender) + 1) };
      // A value of 0xffff wraps to 0: the comparison is skipped instead of dividing by zero.
      if (own && other / own > 3) return 0;
    }
  }
  return 1;
}

void opeu_order_attack_present(uint8_t ds[65536], HostEvent event) {
  Call(ds, event, OrderService::PENDING_FINISH, SHOW_DONE);
}

// Starts the battle against `name` at x, y: needs supplies and ammunition; a player's division the computer does not
// run asks for confirmation; otherwise the attack must be allowed and is reported.
uint16_t opeu_order_attack_start(uint8_t ds[65536], HostEvent event, uint16_t name, uint8_t x, uint8_t y) {
  auto& data{ Data(ds) };
  if (!Call(ds, event, OrderService::MOVE_SUPPLIED)) {
    Call(ds, event, OrderService::UNIT_REPORT, data.current_unit_ptr, Near(data, data.order_no_fuel));
    Restore(ds, event);
    return 0;
  }
  if (CurrentUnit(data).resources[0] == 0) {
    Call(ds, event, OrderService::UNIT_REPORT, data.current_unit_ptr, Near(data, data.order_no_ammo));
    Restore(ds, event);
    return 0;
  }
  ServiceLocals const locals{ START_BYTES };
  auto const          local{ [&](ServiceLocal offset) { return locals.At(static_cast<std::uint16_t>(START_BYTES - offset)); } };
  auto const          text{ local(START_TEXT) };
  if (data.side_control[data.current_side] == SIDE_PLAYED && !(Record<Group>(data, data.current_group_ptr).flags & Bits(GroupFlag::AI_MARK))
      && Call(ds, event, OrderService::UNIT_SELECTION_ELIGIBLE, data.current_unit_ptr)) {
    Call(ds, event, OrderService::UNIT_REPORT, data.current_unit_ptr, Near(data, data.order_prepared));
    Call(ds, event, OrderService::REQUEST_FORMAT, text, Near(data, data.order_ask_attack), name);
    auto const confirmed{ Call(ds, event, OrderService::CONFIRM_MESSAGE, text, data.context_group_record) };
    if (confirmed) Call(ds, event, OrderService::DAILY_SHOW, BATTLE_PICTURE, data.current_side, ATTACK_PRESENT, ORDER_SEGMENT);
    return confirmed;
  }
  if (!opeu_order_attack_allowed(ds, event, x, y)) return 0;
  Call(ds, event, OrderService::REQUEST_FORMAT, text, Near(data, data.order_begin_attack), name);
  Call(ds, event, OrderService::UNIT_REPORT, data.current_unit_ptr, text);
  return 1;
}

uint8_t opeu_order_occupy_loss(uint8_t ds[65536], HostEvent event) {
  return Loss(ds, event, 65);
}

void opeu_order_occupied_present(uint8_t ds[65536], HostEvent event) {
  Call(ds, event, OrderService::WAIT_UNITS, OCCUPIED_WAIT);
}

// The occupy order against the enemy objective of the order. An objective without arms changes hands when the regiment
// moves in (its owner bit is flipped for the move and flipped back when the cell did not become the side's).
uint16_t opeu_order_occupy_run(uint8_t ds[65536], HostEvent event) {
  auto&               data{ Data(ds) };
  auto const          enemy{ static_cast<std::uint8_t>(Call(ds, event, OrderService::OPPOSITE_SIDE, data.current_side)) };
  ServiceLocals const locals{ OCCUPY_BYTES };
  auto const          local{ [&](ServiceLocal offset) { return locals.At(static_cast<std::uint16_t>(OCCUPY_BYTES - offset)); } };
  auto const          at_x{ local(OCCUPY_X) };
  auto const          at_y{ local(OCCUPY_Y) };
  auto const          objective{ Call(ds, event, OrderService::MOVE_OBJECTIVE_TARGET, at_x, at_y, enemy) };
  if (!objective) return 1;
  if (!Call(ds, event, OrderService::MOVE_ADJACENT, Byte(ds, at_x), Byte(ds, at_y))) return 0;
  auto const loss{ opeu_order_occupy_loss(ds, event) };
  if (!Call(ds, event, OrderService::MOVE_LEVEL_LOWER, CurrentOrder(data), loss)) return 1;
  if (static_cast<std::uint8_t>(Call(ds, event, OrderService::CELL_SIDE, Byte(ds, at_x), Byte(ds, at_y))) == NO_SIDE
      && !Call(ds, event, OrderService::OBJECTIVE_QUANTITY, objective, ALL_ARMS, local(OCCUPY_COUNT))) {
    auto& target{ Record<Objective>(data, objective) };
    target.flags ^= Bits(ObjectiveFlag::SIDE_1);
    Call(ds, event, OrderService::MOVE_UNIT, 1);
    if (static_cast<std::uint8_t>(Call(ds, event, OrderService::CELL_SIDE, Byte(ds, at_x), Byte(ds, at_y))) != data.current_side) {
      target.flags ^= Bits(ObjectiveFlag::SIDE_1);
      return 1;
    }
    auto const text{ local(OCCUPY_TEXT) };
    Call(ds, event, OrderService::REQUEST_FORMAT, text, Near(data, data.order_occupied), objective);
    Call(ds, event, OrderService::UNIT_REPORT, data.current_unit_ptr, text);
    Call(ds, event, OrderService::DAILY_SHOW, OCCUPIED_PICTURE, data.current_side, OCCUPIED_PRESENT, ORDER_SEGMENT);
    Restore(ds, event);
    Call(ds, event, OrderService::SIDE_GROUPS_RETARGET, 0);
    Call(ds, event, OrderService::SIDE_GROUPS_RETARGET, 1);
    return 1;
  }
  if (opeu_order_attack_start(ds, event, objective, Byte(ds, at_x), Byte(ds, at_y))) Engage(data, Byte(ds, at_x), Byte(ds, at_y));
  return 1;
}

uint8_t opeu_order_attack_loss(uint8_t ds[65536], HostEvent event) {
  return Loss(ds, event, 60);
}

// The attack order against the enemy regiment of the order, labelled with its side and name.
uint16_t opeu_order_attack_run(uint8_t ds[65536], HostEvent event) {
  auto&               data{ Data(ds) };
  ServiceLocals const locals{ ATTACK_BYTES };
  auto const          local{ [&](ServiceLocal offset) { return locals.At(static_cast<std::uint16_t>(ATTACK_BYTES - offset)); } };
  auto const          at_x{ local(ATTACK_X) };
  auto const          at_y{ local(ATTACK_Y) };
  auto const          unit{ Call(ds, event, OrderService::MOVE_UNIT_TARGET, at_x, at_y) };
  if (!unit) return 1;
  if (!Call(ds, event, OrderService::MOVE_ADJACENT, Byte(ds, at_x), Byte(ds, at_y))) return 0;
  auto const loss{ opeu_order_attack_loss(ds, event) };
  if (!Call(ds, event, OrderService::MOVE_LEVEL_LOWER, CurrentOrder(data), loss)) return 1;
  auto const name{ Call(ds, event, OrderService::UNIT_NAME, unit, local(ATTACK_NAME)) };
  auto const side_name{ data.side_names[Call(ds, event, OrderService::ABS16_BITS, static_cast<std::uint16_t>(data.current_side - 1))] };
  auto const text{ local(ATTACK_TEXT) };
  Call(ds, event, OrderService::REQUEST_FORMAT, text, Near(data, data.order_unit_label), data.current_side ? SIDE1_COLOUR : SIDE0_COLOUR, side_name, name);
  if (opeu_order_attack_start(ds, event, text, Byte(ds, at_x), Byte(ds, at_y))) Engage(data, Byte(ds, at_x), Byte(ds, at_y));
  return 1;
}

// The occupy and attack orders' calls are order services (all up to UNIT_AVERAGE9, pending_finish and wait_units).
uint16_t svc_ordattk(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  if ((id > OrderService::UNIT_AVERAGE9 && id != OrderService::PENDING_FINISH && id != OrderService::WAIT_UNITS) || !svc_order_handler(ds, id, args, count, &r)) port_unhandled("ordattk", id, count);
  return r;
}

auto port_routine_opeu_order_attack_present(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_order_attack_present(ds, svc_ordattk);
  return 0;
}

auto port_routine_opeu_order_attack_run(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_order_attack_run(ds, svc_ordattk);
}

auto port_routine_opeu_order_occupied_present(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_order_occupied_present(ds, svc_ordattk);
  return 0;
}

auto port_routine_opeu_order_occupy_run(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_order_occupy_run(ds, svc_ordattk);
}
