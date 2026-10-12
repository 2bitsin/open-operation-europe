// The occupy and attack orders. Next to its target the regiment lowers the order's progress by a loss roll; when it
// runs out, an empty enemy objective is taken, otherwise the regiment opens a battle (after the player confirms it for
// a player's division that the computer does not run). The frame locals the services write through their addresses
// stay in the DS stack frame.
#include "types.hpp"
#include "units/ordattk.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "common/order.hpp"
#include "units/order-handler.hpp"
#include "common/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"
#include "units/engineer.hpp"

using namespace opeu::game;

namespace {
// Frame locals, as offsets below the frame pointer.
enum FrameLocal : std::uint16_t {
  STEP_X        = 2, // attack_allowed: the neighbour cell
  STEP_Y        = 4,
  OCCUPY_X      = 6, // occupy: the objective's cell
  OCCUPY_Y      = 0x6c,
  OCCUPY_COUNT  = 4,    // occupy: objective_quantity's count
  OCCUPY_TEXT   = 0x6a, // occupy: the report
  ATTACK_X      = 4,    // attack: the target regiment's cell
  ATTACK_Y      = 0x30,
  ATTACK_NAME   = 0x10, // attack: the target's name
  ATTACK_TEXT   = 0x2e, // attack: the target's label
  START_TEXT    = 0x66, // attack_start: the confirmation and report text
  START_FRAME   = 0x72, // occupy: attack_start's frame below the handler's
  ALLOWED_FRAME = 0x72, // attack_start: attack_allowed's frame below its own
  ATTACK_START_FRAME = 0x36, // attack: attack_start's frame below the handler's
};

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
  return Serve(ds, event, OrderService::RAND_BOUNDED, RollBound(Data(ds)));
}

auto Restore(std::uint8_t* ds, HostEvent event) -> void {
  Serve(ds, event, OrderService::RESTORE_UNIT_ORDER, Data(ds).current_unit_ptr);
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
auto Loss(std::uint8_t* ds, HostEvent event, std::uint16_t divisor)
    -> std::uint8_t {
  auto&               data{ Data(ds) };
  auto const          person{ Serve(ds, event, OrderService::UNIT_PERSON,
                                    data.current_unit_ptr) };
  std::uint16_t const random{ Random(ds, event) };
  std::uint16_t const average10{ Serve(ds, event, OrderService::UNIT_AVERAGE10,
                                       data.current_unit_ptr) };
  std::uint16_t const average9{ Serve(ds, event, OrderService::UNIT_AVERAGE9,
                                      data.current_unit_ptr) };
  std::uint16_t const average11{ Serve(ds, event, OrderService::UNIT_AVERAGE11,
                                       data.current_unit_ptr) };
  auto const&         general{ Record<Person>(data, person) };
  return static_cast<std::uint8_t>(
      average11 / 19 -
      static_cast<std::uint16_t>(general.leadership + average10 + average9) /
          divisor -
      general.bravery / 80 + random + 6);
}
} // namespace

// Whether the current regiment may attack the cell x, y. A regiment whose average value11 reaches 70 + rand(30)
// gives up its order; no attack starts while a friendly neighbour's occupy or attack is under way, nor against a
// defending regiment worth more than three times as much.
uint16_t opeu_order_attack_allowed(uint8_t ds[65536], HostEvent event,
                                   uint16_t frame, uint8_t x, uint8_t y) {
  auto&      data{ Data(ds) };
  auto const average{ Serve(ds, event, OrderService::UNIT_AVERAGE11,
                            data.current_unit_ptr) };
  if (average >= static_cast<std::uint16_t>(
                     Serve(ds, event, OrderService::RAND_BOUNDED, 30) + 70)) {
    Restore(ds, event);
    return 0;
  }
  auto const step_x{ Local(frame, STEP_X) };
  auto const step_y{ Local(frame, STEP_Y) };
  for (std::uint8_t direction{ 0 }; direction < 8; ++direction) {
    Byte(ds, step_x) = x;
    Byte(ds, step_y) = y;
    if (!Serve(ds, event, OrderService::GRID_STEP, step_x, step_y, direction))
      continue;
    auto const unit{ Serve(ds, event, OrderService::FIND_UNIT_AT,
                           Byte(ds, step_x), Byte(ds, step_y)) };
    if (!unit ||
        static_cast<std::uint8_t>(Serve(ds, event, OrderService::UNIT_SIDE,
                                        unit)) != data.current_side)
      continue;
    auto const& order{ Record<Unit>(data, unit).orders[0] };
    if ((order.kind == Kind(UnitOrder::OCCUPY) ||
         order.kind == Kind(UnitOrder::ATTACK)) &&
        order.progress)
      return 0;
  }
  auto const defender{ Serve(ds, event, OrderService::FIND_UNIT_AT, x, y) };
  if (defender) {
    auto const& order{ Record<Unit>(data, defender).orders[0] };
    if (order.kind == Kind(UnitOrder::DEFEND) && order.progress == 0) {
      auto const own{ static_cast<std::uint16_t>(
          Serve(ds, event, OrderService::UNIT_VALUE, data.current_unit_ptr) +
          1) };
      auto const other{ static_cast<std::uint16_t>(
          Serve(ds, event, OrderService::UNIT_VALUE, defender) + 1) };
      // A value of 0xffff wraps to 0: the comparison is skipped instead of dividing by zero.
      if (own && other / own > 3)
        return 0;
    }
  }
  return 1;
}

void opeu_order_attack_present(uint8_t ds[65536], HostEvent event) {
  Serve(ds, event, OrderService::PENDING_FINISH, SHOW_DONE);
}

// Starts the battle against `name` at x, y: needs supplies and ammunition; a player's division the computer does not
// run asks for confirmation; otherwise the attack must be allowed and is reported.
uint16_t opeu_order_attack_start(uint8_t ds[65536], HostEvent event,
                                 uint16_t frame, uint16_t name, uint8_t x,
                                 uint8_t y) {
  auto& data{ Data(ds) };
  if (!Serve(ds, event, OrderService::MOVE_SUPPLIED)) {
    Serve(ds, event, OrderService::UNIT_REPORT, data.current_unit_ptr,
          Near(data, data.order_no_fuel));
    Restore(ds, event);
    return 0;
  }
  if (CurrentUnit(data).resources[0] == 0) {
    Serve(ds, event, OrderService::UNIT_REPORT, data.current_unit_ptr,
          Near(data, data.order_no_ammo));
    Restore(ds, event);
    return 0;
  }
  auto const text{ Local(frame, START_TEXT) };
  if (data.side_control[data.current_side] == PLAYER &&
      !(Record<Group>(data, data.current_group_ptr).flags &
        Bits(GroupFlag::AI_MARK)) &&
      Serve(ds, event, OrderService::UNIT_SELECTION_ELIGIBLE,
            data.current_unit_ptr)) {
    Serve(ds, event, OrderService::UNIT_REPORT, data.current_unit_ptr,
          Near(data, data.order_prepared));
    Serve(ds, event, OrderService::REQUEST_FORMAT, text,
          Near(data, data.order_ask_attack), name);
    auto const confirmed{ Serve(ds, event, OrderService::CONFIRM_MESSAGE, text,
                                data.context_group_record) };
    if (confirmed)
      Serve(ds, event, OrderService::DAILY_SHOW, BATTLE_PICTURE,
            data.current_side, ATTACK_PRESENT, ORDER_SEGMENT);
    return confirmed;
  }
  if (!opeu_order_attack_allowed(ds, event, Local(frame, ALLOWED_FRAME), x, y))
    return 0;
  Serve(ds, event, OrderService::REQUEST_FORMAT, text,
        Near(data, data.order_begin_attack), name);
  Serve(ds, event, OrderService::UNIT_REPORT, data.current_unit_ptr, text);
  return 1;
}

uint8_t opeu_order_occupy_loss(uint8_t ds[65536], HostEvent event) {
  return Loss(ds, event, 65);
}

void opeu_order_occupied_present(uint8_t ds[65536], HostEvent event) {
  Serve(ds, event, OrderService::WAIT_UNITS, OCCUPIED_WAIT);
}

// The occupy order against the enemy objective of the order. An objective without arms changes hands when the regiment
// moves in (its owner bit is flipped for the move and flipped back when the cell did not become the side's).
uint16_t opeu_order_occupy_run(uint8_t ds[65536], HostEvent event,
                               uint16_t frame) {
  auto&      data{ Data(ds) };
  auto const enemy{ static_cast<std::uint8_t>(
      Serve(ds, event, OrderService::OPPOSITE_SIDE, data.current_side)) };
  auto const at_x{ Local(frame, OCCUPY_X) };
  auto const at_y{ Local(frame, OCCUPY_Y) };
  auto const objective{ Serve(ds, event, OrderService::MOVE_OBJECTIVE_TARGET,
                              at_x, at_y, enemy) };
  if (!objective)
    return 1;
  if (!Serve(ds, event, OrderService::MOVE_ADJACENT, Byte(ds, at_x),
             Byte(ds, at_y)))
    return 0;
  auto const loss{ opeu_order_occupy_loss(ds, event) };
  if (!Serve(ds, event, OrderService::MOVE_LEVEL_LOWER, CurrentOrder(data),
             loss))
    return 1;
  if (static_cast<std::uint8_t>(Serve(ds, event, OrderService::CELL_SIDE,
                                      Byte(ds, at_x), Byte(ds, at_y))) ==
          NO_SIDE &&
      !Serve(ds, event, OrderService::OBJECTIVE_QUANTITY, objective, ALL_ARMS,
             Local(frame, OCCUPY_COUNT))) {
    auto& target{ Record<Objective>(data, objective) };
    target.flags ^= Bits(ObjectiveFlag::SIDE_1);
    Serve(ds, event, OrderService::MOVE_UNIT, 1);
    if (static_cast<std::uint8_t>(Serve(ds, event, OrderService::CELL_SIDE,
                                        Byte(ds, at_x), Byte(ds, at_y))) !=
        data.current_side) {
      target.flags ^= Bits(ObjectiveFlag::SIDE_1);
      return 1;
    }
    auto const text{ Local(frame, OCCUPY_TEXT) };
    Serve(ds, event, OrderService::REQUEST_FORMAT, text,
          Near(data, data.order_occupied), objective);
    Serve(ds, event, OrderService::UNIT_REPORT, data.current_unit_ptr, text);
    Serve(ds, event, OrderService::DAILY_SHOW, OCCUPIED_PICTURE,
          data.current_side, OCCUPIED_PRESENT, ORDER_SEGMENT);
    Restore(ds, event);
    Serve(ds, event, OrderService::SIDE_GROUPS_RETARGET, 0);
    Serve(ds, event, OrderService::SIDE_GROUPS_RETARGET, 1);
    return 1;
  }
  if (opeu_order_attack_start(ds, event, Local(frame, START_FRAME), objective,
                              Byte(ds, at_x), Byte(ds, at_y)))
    Engage(data, Byte(ds, at_x), Byte(ds, at_y));
  return 1;
}

uint8_t opeu_order_attack_loss(uint8_t ds[65536], HostEvent event) {
  return Loss(ds, event, 60);
}

// The attack order against the enemy regiment of the order, labelled with its side and name.
uint16_t opeu_order_attack_run(uint8_t ds[65536], HostEvent event,
                               uint16_t frame) {
  auto&      data{ Data(ds) };
  auto const at_x{ Local(frame, ATTACK_X) };
  auto const at_y{ Local(frame, ATTACK_Y) };
  auto const unit{ Serve(ds, event, OrderService::MOVE_UNIT_TARGET, at_x,
                         at_y) };
  if (!unit)
    return 1;
  if (!Serve(ds, event, OrderService::MOVE_ADJACENT, Byte(ds, at_x),
             Byte(ds, at_y)))
    return 0;
  auto const loss{ opeu_order_attack_loss(ds, event) };
  if (!Serve(ds, event, OrderService::MOVE_LEVEL_LOWER, CurrentOrder(data),
             loss))
    return 1;
  auto const name{ Serve(ds, event, OrderService::UNIT_NAME, unit,
                         Local(frame, ATTACK_NAME)) };
  auto const side_name{
    data.side_names[Serve(ds, event, OrderService::ABS16_BITS,
                          static_cast<std::uint16_t>(data.current_side - 1))]
  };
  auto const text{ Local(frame, ATTACK_TEXT) };
  Serve(ds, event, OrderService::REQUEST_FORMAT, text,
        Near(data, data.order_unit_label),
        data.current_side ? SIDE1_COLOUR : SIDE0_COLOUR, side_name, name);
  if (opeu_order_attack_start(ds, event, Local(frame, ATTACK_START_FRAME), text,
                              Byte(ds, at_x), Byte(ds, at_y)))
    Engage(data, Byte(ds, at_x), Byte(ds, at_y));
  return 1;
}

// The occupy and attack orders' calls are order services (all up to UNIT_AVERAGE9, pending_finish and wait_units).
uint16_t svc_ordattk(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count) {
  uint16_t r = 0;
  if ((id > OrderService::UNIT_AVERAGE9 && id != OrderService::PENDING_FINISH &&
       id != OrderService::WAIT_UNITS) ||
      !svc_order_handler(ds, id, args, count, &r))
    port_unhandled("ordattk", id, count);
  return r;
}

auto port_routine_opeu_order_attack_present(std::uint8_t*        ds,
                                            std::uint16_t const* args,
                                            unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_order_attack_present(ds, svc_ordattk);
  return 0;
}

auto port_routine_opeu_order_attack_run(std::uint8_t*        ds,
                                        std::uint16_t const* args,
                                        unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return WithStack(SERVICE_STACK, [&](std::uint16_t sp) {
    return opeu_order_attack_run(ds, svc_ordattk,
                                 static_cast<std::uint16_t>(sp - 2));
  });
}

auto port_routine_opeu_order_occupied_present(std::uint8_t*        ds,
                                              std::uint16_t const* args,
                                              unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_order_occupied_present(ds, svc_ordattk);
  return 0;
}

auto port_routine_opeu_order_occupy_run(std::uint8_t*        ds,
                                        std::uint16_t const* args,
                                        unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return WithStack(SERVICE_STACK, [&](std::uint16_t sp) {
    return opeu_order_occupy_run(ds, svc_ordattk,
                                 static_cast<std::uint16_t>(sp - 2));
  });
}
