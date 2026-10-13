// The occupy and attack orders. Next to its target the regiment lowers the order's progress by a loss roll; when it
// runs out, an empty enemy objective is taken, otherwise the regiment opens a battle (after the player confirms it for
// a player's division that the computer does not run). The locals the services write through their addresses stay in
// the data segment, each routine's in its own reserve.
#include "units/ordattk.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/order.hpp"
#include "core/dialogs.hpp"
#include "core/limits.hpp"
#include "core/objrank.hpp"
#include "core/recindex.hpp"
#include "core/sideutil.hpp"
#include "core/textout.hpp"
#include "io/waitkey.hpp"
#include "map/movesup.hpp"
#include "map/movetgt.hpp"
#include "map/moveunit.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/order-handler.hpp"
#include "units/orders.hpp"
#include "units/stats.hpp"
#include "units/unitname.hpp"
#include "units/unitpers.hpp"
#include "units/value.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/engineer.hpp"
#include <array>

using namespace opeu::game;

namespace {
// The locals the services read and write, as offsets below the top of the routine's reserve.
enum ServiceLocal : std::uint16_t {
  STEP_X      = 2, // attack_allowed: the neighbour cell
  STEP_Y      = 4,
  OCCUPY_X    = 6, // occupy: the objective's cell
  OCCUPY_Y    = 0x6c,
  OCCUPY_TEXT = 0x6a, // occupy: the report
  ATTACK_X    = 4,    // attack: the target regiment's cell
  ATTACK_Y    = 0x30,
  ATTACK_NAME = 0x10, // attack: the target's name
  ATTACK_TEXT = 0x2e, // attack: the target's label
  START_TEXT  = 0x66, // attack_start: the confirmation and report text
};
// The reserves: each routine's deepest local.
constexpr std::uint16_t ALLOWED_BYTES{ STEP_Y };
constexpr std::uint16_t START_BYTES{ START_TEXT };
constexpr std::uint16_t OCCUPY_BYTES{ OCCUPY_Y };
constexpr std::uint16_t ATTACK_BYTES{ ATTACK_Y };

// The pictures of the day's show.
constexpr std::uint16_t BATTLE_PICTURE{ 0x1d7 };
constexpr std::uint16_t OCCUPIED_PICTURE{ 0xa2 };
// The ticks wait_units waits after an occupation.
constexpr std::uint16_t OCCUPIED_WAIT{ 0x1e };
// The colour codes of the side names in a regiment label.
constexpr std::uint16_t SIDE0_COLOUR{ '5' };
constexpr std::uint16_t SIDE1_COLOUR{ '6' };

auto Random(Game& game) -> std::uint16_t {
  return port_rand_bounded(game, RollBound(Data(game)));
}

auto Restore(Game& game) -> void {
  opeu_restore_unit_order(game, Data(game).current_unit_ptr);
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
auto Loss(Game& game, std::uint16_t divisor) -> std::uint8_t {
  auto&               data{ Data(game) };
  auto const          person{ opeu_unit_person(game, data.current_unit_ptr) };
  std::uint16_t const random{ Random(game) };
  std::uint16_t const average10{ opeu_unit_average10(game, data.current_unit_ptr) };
  std::uint16_t const average9{ opeu_unit_average9(game, data.current_unit_ptr) };
  std::uint16_t const average11{ opeu_unit_average11(game, data.current_unit_ptr) };
  auto const&         general{ Record<Person>(data, person) };
  return static_cast<std::uint8_t>(average11 / 19 - static_cast<std::uint16_t>(general.leadership + average10 + average9) / divisor - general.bravery / 80 + random + 6);
}
} // namespace

// Whether the current regiment may attack the cell x, y. A regiment whose average value11 reaches 70 + rand(30)
// gives up its order; no attack starts while a friendly neighbour's occupy or attack is under way, nor against a
// defending regiment worth more than three times as much.
uint16_t opeu_order_attack_allowed(Game& game, uint8_t x, uint8_t y) {
  auto&      data{ Data(game) };
  auto const average{ opeu_unit_average11(game, data.current_unit_ptr) };
  if (average >= static_cast<std::uint16_t>(port_rand_bounded(game, 30) + 70)) {
    Restore(game);
    return 0;
  }
  ServiceLocals const locals{ ALLOWED_BYTES };
  auto const          local{ [&](ServiceLocal offset) { return locals.At(static_cast<std::uint16_t>(ALLOWED_BYTES - offset)); } };
  auto const          step_x{ local(STEP_X) };
  auto const          step_y{ local(STEP_Y) };
  for (std::uint8_t direction{ 0 }; direction < 8; ++direction) {
    Byte(game, step_x) = x;
    Byte(game, step_y) = y;
    if (!opeu_grid_step(game.State().Bytes(step_x, 1).data(), game.State().Bytes(step_y, 1).data(), static_cast<uint8_t>(direction))) continue;
    auto const unit{ opeu_find_unit_at(game, static_cast<uint8_t>(Byte(game, step_x)), static_cast<uint8_t>(Byte(game, step_y))) };
    if (!unit || static_cast<std::uint8_t>(opeu_unit_side(unit)) != data.current_side) continue;
    auto const& order{ Record<Unit>(data, unit).orders[0] };
    if ((order.kind == Kind(UnitOrder::OCCUPY) || order.kind == Kind(UnitOrder::ATTACK)) && order.progress) return 0;
  }
  auto const defender{ opeu_find_unit_at(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y)) };
  if (defender) {
    auto const& order{ Record<Unit>(data, defender).orders[0] };
    if (order.kind == Kind(UnitOrder::DEFEND) && order.progress == 0) {
      auto const own{ static_cast<std::uint16_t>(opeu_unit_value(game, data.current_unit_ptr) + 1) };
      auto const other{ static_cast<std::uint16_t>(opeu_unit_value(game, defender) + 1) };
      // A value of 0xffff wraps to 0: the comparison is skipped instead of dividing by zero.
      if (own && other / own > 3) return 0;
    }
  }
  return 1;
}

void opeu_order_attack_present(Game& game) {
  opeu_pending_finish(game, static_cast<uint8_t>(SHOW_DONE));
}

// Starts the battle against `name` at x, y: needs supplies and ammunition; a player's division the computer does not
// run asks for confirmation; otherwise the attack must be allowed and is reported.
uint16_t opeu_order_attack_start(Game& game, uint16_t name, uint8_t x, uint8_t y) {
  auto& data{ Data(game) };
  if (!opeu_move_supplied(game)) {
    opeu_unit_report(game, static_cast<const DialogHost*>(port_dialogs_host()), data.current_unit_ptr, Near(data, data.order_no_fuel));
    Restore(game);
    return 0;
  }
  if (CurrentUnit(data).resources[0] == 0) {
    opeu_unit_report(game, static_cast<const DialogHost*>(port_dialogs_host()), data.current_unit_ptr, Near(data, data.order_no_ammo));
    Restore(game);
    return 0;
  }
  ServiceLocals const locals{ START_BYTES };
  auto const          local{ [&](ServiceLocal offset) { return locals.At(static_cast<std::uint16_t>(START_BYTES - offset)); } };
  auto const          text{ local(START_TEXT) };
  if (data.side_control[data.current_side] == SIDE_PLAYED && !(Record<Group>(data, data.current_group_ptr).flags & Bits(GroupFlag::AI_MARK))
      && opeu_unit_selection_eligible(game, data.current_unit_ptr)) {
    opeu_unit_report(game, static_cast<const DialogHost*>(port_dialogs_host()), data.current_unit_ptr, Near(data, data.order_prepared));
    opeu_request_format(game, std::array<std::uint16_t, 3>{ text, Near(data, data.order_ask_attack), name });
    auto const confirmed{ opeu_confirm_message(game, static_cast<const DialogHost*>(port_dialogs_host()), text, data.context_group_record) };
    if (confirmed) opeu_daily_show(game, static_cast<const DialogHost*>(port_dialogs_host()), BATTLE_PICTURE, static_cast<uint8_t>(data.current_side), Presenter::ATTACK);
    return confirmed;
  }
  if (!opeu_order_attack_allowed(game, x, y)) return 0;
  opeu_request_format(game, std::array<std::uint16_t, 3>{ text, Near(data, data.order_begin_attack), name });
  opeu_unit_report(game, static_cast<const DialogHost*>(port_dialogs_host()), data.current_unit_ptr, text);
  return 1;
}

uint8_t opeu_order_occupy_loss(Game& game) {
  return Loss(game, 65);
}

void opeu_order_occupied_present(Game& game) {
  opeu_delay_units(game, static_cast<uint8_t>(OCCUPIED_WAIT));
}

// The occupy order against the enemy objective of the order. An objective without arms changes hands when the regiment
// moves in (its owner bit is flipped for the move and flipped back when the cell did not become the side's).
uint16_t opeu_order_occupy_run(Game& game) {
  auto&               data{ Data(game) };
  auto const          enemy{ static_cast<std::uint8_t>(opeu_opposite_side(static_cast<uint8_t>(data.current_side))) };
  ServiceLocals const locals{ OCCUPY_BYTES };
  auto const          local{ [&](ServiceLocal offset) { return locals.At(static_cast<std::uint16_t>(OCCUPY_BYTES - offset)); } };
  auto const          at_x{ local(OCCUPY_X) };
  auto const          at_y{ local(OCCUPY_Y) };
  auto const          objective{ opeu_move_objective_target(game, at_x, at_y, static_cast<uint8_t>(enemy)) };
  if (!objective) return 1;
  if (!opeu_move_adjacent(game, static_cast<uint8_t>(Byte(game, at_x)), static_cast<uint8_t>(Byte(game, at_y)))) return 0;
  auto const loss{ opeu_order_occupy_loss(game) };
  if (!opeu_move_level_lower(game, CurrentOrder(data), static_cast<uint8_t>(loss))) return 1;
  if (static_cast<std::uint8_t>(opeu_cell_side(game, static_cast<uint8_t>(Byte(game, at_x)), static_cast<uint8_t>(Byte(game, at_y)))) == NO_SIDE
      && !ObjectiveQuantity(data, objective, ANY_CATEGORY).total) {
    auto& target{ Record<Objective>(data, objective) };
    target.flags ^= Bits(ObjectiveFlag::SIDE_1);
    opeu_move_unit(game, 1);
    if (static_cast<std::uint8_t>(opeu_cell_side(game, static_cast<uint8_t>(Byte(game, at_x)), static_cast<uint8_t>(Byte(game, at_y)))) != data.current_side) {
      target.flags ^= Bits(ObjectiveFlag::SIDE_1);
      return 1;
    }
    auto const text{ local(OCCUPY_TEXT) };
    opeu_request_format(game, std::array<std::uint16_t, 3>{ text, Near(data, data.order_occupied), objective });
    opeu_unit_report(game, static_cast<const DialogHost*>(port_dialogs_host()), data.current_unit_ptr, text);
    opeu_daily_show(game, static_cast<const DialogHost*>(port_dialogs_host()), OCCUPIED_PICTURE, static_cast<uint8_t>(data.current_side), Presenter::OCCUPIED);
    Restore(game);
    opeu_side_groups_retarget(game, 0);
    opeu_side_groups_retarget(game, 1);
    return 1;
  }
  if (opeu_order_attack_start(game, objective, Byte(game, at_x), Byte(game, at_y))) Engage(data, Byte(game, at_x), Byte(game, at_y));
  return 1;
}

uint8_t opeu_order_attack_loss(Game& game) {
  return Loss(game, 60);
}

// The attack order against the enemy regiment of the order, labelled with its side and name.
uint16_t opeu_order_attack_run(Game& game) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ ATTACK_BYTES };
  auto const          local{ [&](ServiceLocal offset) { return locals.At(static_cast<std::uint16_t>(ATTACK_BYTES - offset)); } };
  auto const          at_x{ local(ATTACK_X) };
  auto const          at_y{ local(ATTACK_Y) };
  auto const          unit{ opeu_move_unit_target(game, at_x, at_y) };
  if (!unit) return 1;
  if (!opeu_move_adjacent(game, static_cast<uint8_t>(Byte(game, at_x)), static_cast<uint8_t>(Byte(game, at_y)))) return 0;
  auto const loss{ opeu_order_attack_loss(game) };
  if (!opeu_move_level_lower(game, CurrentOrder(data), static_cast<uint8_t>(loss))) return 1;
  auto const name{ opeu_unit_name(game, unit, local(ATTACK_NAME)) };
  auto const side_name{ data.side_names[opeu_abs16_bits(static_cast<std::uint16_t>(data.current_side - 1))] };
  auto const text{ local(ATTACK_TEXT) };
  opeu_request_format(game, std::array<std::uint16_t, 5>{ text, Near(data, data.order_unit_label), data.current_side ? SIDE1_COLOUR : SIDE0_COLOUR, side_name, name });
  if (opeu_order_attack_start(game, text, Byte(game, at_x), Byte(game, at_y))) Engage(data, Byte(game, at_x), Byte(game, at_y));
  return 1;
}
