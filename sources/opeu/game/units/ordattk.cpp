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

#include "common/game.hpp"
#include "link/glue.hpp"
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

auto Random(Game& game, HostEvent event) -> std::uint16_t {
  return Call(game, event, OrderService::RAND_BOUNDED, RollBound(Data(game)));
}

auto Restore(Game& game, HostEvent event) -> void {
  Call(game, event, OrderService::RESTORE_UNIT_ORDER, Data(game).current_unit_ptr);
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
auto Loss(Game& game, HostEvent event, std::uint16_t divisor) -> std::uint8_t {
  auto&               data{ Data(game) };
  auto const          person{ Call(game, event, OrderService::UNIT_PERSON, data.current_unit_ptr) };
  std::uint16_t const random{ Random(game, event) };
  std::uint16_t const average10{ Call(game, event, OrderService::UNIT_AVERAGE10, data.current_unit_ptr) };
  std::uint16_t const average9{ Call(game, event, OrderService::UNIT_AVERAGE9, data.current_unit_ptr) };
  std::uint16_t const average11{ Call(game, event, OrderService::UNIT_AVERAGE11, data.current_unit_ptr) };
  auto const&         general{ Record<Person>(data, person) };
  return static_cast<std::uint8_t>(average11 / 19 - static_cast<std::uint16_t>(general.leadership + average10 + average9) / divisor - general.bravery / 80 + random + 6);
}
} // namespace

// Whether the current regiment may attack the cell x, y. A regiment whose average value11 reaches 70 + rand(30)
// gives up its order; no attack starts while a friendly neighbour's occupy or attack is under way, nor against a
// defending regiment worth more than three times as much.
uint16_t opeu_order_attack_allowed(Game& game, HostEvent event, uint8_t x, uint8_t y) {
  auto&      data{ Data(game) };
  auto const average{ Call(game, event, OrderService::UNIT_AVERAGE11, data.current_unit_ptr) };
  if (average >= static_cast<std::uint16_t>(Call(game, event, OrderService::RAND_BOUNDED, 30) + 70)) {
    Restore(game, event);
    return 0;
  }
  ServiceLocals const locals{ ALLOWED_BYTES };
  auto const          local{ [&](ServiceLocal offset) { return locals.At(static_cast<std::uint16_t>(ALLOWED_BYTES - offset)); } };
  auto const          step_x{ local(STEP_X) };
  auto const          step_y{ local(STEP_Y) };
  for (std::uint8_t direction{ 0 }; direction < 8; ++direction) {
    Byte(game, step_x) = x;
    Byte(game, step_y) = y;
    if (!Call(game, event, OrderService::GRID_STEP, step_x, step_y, direction)) continue;
    auto const unit{ Call(game, event, OrderService::FIND_UNIT_AT, Byte(game, step_x), Byte(game, step_y)) };
    if (!unit || static_cast<std::uint8_t>(Call(game, event, OrderService::UNIT_SIDE, unit)) != data.current_side) continue;
    auto const& order{ Record<Unit>(data, unit).orders[0] };
    if ((order.kind == Kind(UnitOrder::OCCUPY) || order.kind == Kind(UnitOrder::ATTACK)) && order.progress) return 0;
  }
  auto const defender{ Call(game, event, OrderService::FIND_UNIT_AT, x, y) };
  if (defender) {
    auto const& order{ Record<Unit>(data, defender).orders[0] };
    if (order.kind == Kind(UnitOrder::DEFEND) && order.progress == 0) {
      auto const own{ static_cast<std::uint16_t>(Call(game, event, OrderService::UNIT_VALUE, data.current_unit_ptr) + 1) };
      auto const other{ static_cast<std::uint16_t>(Call(game, event, OrderService::UNIT_VALUE, defender) + 1) };
      // A value of 0xffff wraps to 0: the comparison is skipped instead of dividing by zero.
      if (own && other / own > 3) return 0;
    }
  }
  return 1;
}

void opeu_order_attack_present(Game& game, HostEvent event) {
  Call(game, event, OrderService::PENDING_FINISH, SHOW_DONE);
}

// Starts the battle against `name` at x, y: needs supplies and ammunition; a player's division the computer does not
// run asks for confirmation; otherwise the attack must be allowed and is reported.
uint16_t opeu_order_attack_start(Game& game, HostEvent event, uint16_t name, uint8_t x, uint8_t y) {
  auto& data{ Data(game) };
  if (!Call(game, event, OrderService::MOVE_SUPPLIED)) {
    Call(game, event, OrderService::UNIT_REPORT, data.current_unit_ptr, Near(data, data.order_no_fuel));
    Restore(game, event);
    return 0;
  }
  if (CurrentUnit(data).resources[0] == 0) {
    Call(game, event, OrderService::UNIT_REPORT, data.current_unit_ptr, Near(data, data.order_no_ammo));
    Restore(game, event);
    return 0;
  }
  ServiceLocals const locals{ START_BYTES };
  auto const          local{ [&](ServiceLocal offset) { return locals.At(static_cast<std::uint16_t>(START_BYTES - offset)); } };
  auto const          text{ local(START_TEXT) };
  if (data.side_control[data.current_side] == SIDE_PLAYED && !(Record<Group>(data, data.current_group_ptr).flags & Bits(GroupFlag::AI_MARK))
      && Call(game, event, OrderService::UNIT_SELECTION_ELIGIBLE, data.current_unit_ptr)) {
    Call(game, event, OrderService::UNIT_REPORT, data.current_unit_ptr, Near(data, data.order_prepared));
    Call(game, event, OrderService::REQUEST_FORMAT, text, Near(data, data.order_ask_attack), name);
    auto const confirmed{ Call(game, event, OrderService::CONFIRM_MESSAGE, text, data.context_group_record) };
    if (confirmed) Call(game, event, OrderService::DAILY_SHOW, BATTLE_PICTURE, data.current_side, ATTACK_PRESENT, ORDER_SEGMENT);
    return confirmed;
  }
  if (!opeu_order_attack_allowed(game, event, x, y)) return 0;
  Call(game, event, OrderService::REQUEST_FORMAT, text, Near(data, data.order_begin_attack), name);
  Call(game, event, OrderService::UNIT_REPORT, data.current_unit_ptr, text);
  return 1;
}

uint8_t opeu_order_occupy_loss(Game& game, HostEvent event) {
  return Loss(game, event, 65);
}

void opeu_order_occupied_present(Game& game, HostEvent event) {
  Call(game, event, OrderService::WAIT_UNITS, OCCUPIED_WAIT);
}

// The occupy order against the enemy objective of the order. An objective without arms changes hands when the regiment
// moves in (its owner bit is flipped for the move and flipped back when the cell did not become the side's).
uint16_t opeu_order_occupy_run(Game& game, HostEvent event) {
  auto&               data{ Data(game) };
  auto const          enemy{ static_cast<std::uint8_t>(Call(game, event, OrderService::OPPOSITE_SIDE, data.current_side)) };
  ServiceLocals const locals{ OCCUPY_BYTES };
  auto const          local{ [&](ServiceLocal offset) { return locals.At(static_cast<std::uint16_t>(OCCUPY_BYTES - offset)); } };
  auto const          at_x{ local(OCCUPY_X) };
  auto const          at_y{ local(OCCUPY_Y) };
  auto const          objective{ Call(game, event, OrderService::MOVE_OBJECTIVE_TARGET, at_x, at_y, enemy) };
  if (!objective) return 1;
  if (!Call(game, event, OrderService::MOVE_ADJACENT, Byte(game, at_x), Byte(game, at_y))) return 0;
  auto const loss{ opeu_order_occupy_loss(game, event) };
  if (!Call(game, event, OrderService::MOVE_LEVEL_LOWER, CurrentOrder(data), loss)) return 1;
  if (static_cast<std::uint8_t>(Call(game, event, OrderService::CELL_SIDE, Byte(game, at_x), Byte(game, at_y))) == NO_SIDE
      && !Call(game, event, OrderService::OBJECTIVE_QUANTITY, objective, ALL_ARMS, local(OCCUPY_COUNT))) {
    auto& target{ Record<Objective>(data, objective) };
    target.flags ^= Bits(ObjectiveFlag::SIDE_1);
    Call(game, event, OrderService::MOVE_UNIT, 1);
    if (static_cast<std::uint8_t>(Call(game, event, OrderService::CELL_SIDE, Byte(game, at_x), Byte(game, at_y))) != data.current_side) {
      target.flags ^= Bits(ObjectiveFlag::SIDE_1);
      return 1;
    }
    auto const text{ local(OCCUPY_TEXT) };
    Call(game, event, OrderService::REQUEST_FORMAT, text, Near(data, data.order_occupied), objective);
    Call(game, event, OrderService::UNIT_REPORT, data.current_unit_ptr, text);
    Call(game, event, OrderService::DAILY_SHOW, OCCUPIED_PICTURE, data.current_side, OCCUPIED_PRESENT, ORDER_SEGMENT);
    Restore(game, event);
    Call(game, event, OrderService::SIDE_GROUPS_RETARGET, 0);
    Call(game, event, OrderService::SIDE_GROUPS_RETARGET, 1);
    return 1;
  }
  if (opeu_order_attack_start(game, event, objective, Byte(game, at_x), Byte(game, at_y))) Engage(data, Byte(game, at_x), Byte(game, at_y));
  return 1;
}

uint8_t opeu_order_attack_loss(Game& game, HostEvent event) {
  return Loss(game, event, 60);
}

// The attack order against the enemy regiment of the order, labelled with its side and name.
uint16_t opeu_order_attack_run(Game& game, HostEvent event) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ ATTACK_BYTES };
  auto const          local{ [&](ServiceLocal offset) { return locals.At(static_cast<std::uint16_t>(ATTACK_BYTES - offset)); } };
  auto const          at_x{ local(ATTACK_X) };
  auto const          at_y{ local(ATTACK_Y) };
  auto const          unit{ Call(game, event, OrderService::MOVE_UNIT_TARGET, at_x, at_y) };
  if (!unit) return 1;
  if (!Call(game, event, OrderService::MOVE_ADJACENT, Byte(game, at_x), Byte(game, at_y))) return 0;
  auto const loss{ opeu_order_attack_loss(game, event) };
  if (!Call(game, event, OrderService::MOVE_LEVEL_LOWER, CurrentOrder(data), loss)) return 1;
  auto const name{ Call(game, event, OrderService::UNIT_NAME, unit, local(ATTACK_NAME)) };
  auto const side_name{ data.side_names[Call(game, event, OrderService::ABS16_BITS, static_cast<std::uint16_t>(data.current_side - 1))] };
  auto const text{ local(ATTACK_TEXT) };
  Call(game, event, OrderService::REQUEST_FORMAT, text, Near(data, data.order_unit_label), data.current_side ? SIDE1_COLOUR : SIDE0_COLOUR, side_name, name);
  if (opeu_order_attack_start(game, event, text, Byte(game, at_x), Byte(game, at_y))) Engage(data, Byte(game, at_x), Byte(game, at_y));
  return 1;
}

// The occupy and attack orders' calls are order services (all up to UNIT_AVERAGE9, pending_finish and wait_units).
uint16_t svc_ordattk(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  if ((id > OrderService::UNIT_AVERAGE9 && id != OrderService::PENDING_FINISH && id != OrderService::WAIT_UNITS) || !svc_order_handler(game, id, args, count, &r)) port_unhandled("ordattk", id, count);
  return r;
}

auto port_routine_opeu_order_attack_present(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_order_attack_present(game, svc_ordattk);
  return 0;
}

auto port_routine_opeu_order_attack_run(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_order_attack_run(game, svc_ordattk);
}

auto port_routine_opeu_order_occupied_present(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_order_occupied_present(game, svc_ordattk);
  return 0;
}

auto port_routine_opeu_order_occupy_run(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_order_occupy_run(game, svc_ordattk);
}
