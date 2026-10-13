// The start of a battle: the attacked unit or objective is told and worn, both sides' engaged units are gathered
// (a besieged objective fights as a garrison pseudo-unit built from its equipment), each side decides whether a
// leader takes command and whether the battle is reported, and every engaged element is recorded for the loss
// report at its end (core/notend).
#include "core/notice.hpp"
#include "common/events.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/sideutil.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "core/dialogs.hpp"
#include "core/elemfind.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "core/strings.hpp"
#include "glue.hpp"
#include "io/waitkey.hpp"
#include "map/objeleg.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/support.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/capacity.hpp"
#include "units/stats.hpp"
#include "units/unitname.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

// The services the start of a battle calls through its event (pointers are DS offsets).
enum NoticeService : uint16_t {
  FIND_OBJECTIVE     = 0,  // find_objective_at(x, y)
  FIND_UNIT          = 1,  // find_unit_at(x, y)
  UNIT_CAPACITIES    = 2,  // unit_capacities(unit, result a, result b, result c)
  UNIT_SIDE          = 3,  // unit_side(unit)
  GRID_STEP          = 4,  // grid_step(x pointer, y pointer, direction)
  ELEMENT_UNIT       = 5,  // find_element_unit(element)
  UNIT_ELIGIBLE      = 6,  // unit_selection_eligible(unit)
  OBJECTIVE_ELIGIBLE = 7,  // objective_eligible(objective)
  UNIT_GROUP         = 8,  // unit_group(unit)
  OBJECTIVE_SIDE     = 9,  // objective_side(objective)
  FORMAT             = 10, // request_format(destination, format, arguments...)
  SIDE_RECORD        = 11, // find_side_record(side, kind)
  CONFIRM            = 12, // notice_confirm(text, person)
  PENDING_FINISH     = 13, // pending_finish(value)
  UNIT_SPECIAL       = 14, // unit_special(unit)
  UNIT_NAME          = 15, // unit_name(unit, buffer)
  COPY_STRING        = 16, // string_copy(destination, source)
  UNIT_MESSAGE       = 17, // unit_message(unit, text)
  OBJECTIVE_REPORT   = 18, // notice_objective_report(objective, text)
  DAILY_SHOW         = 19, // daily_show(id, side, callback offset, callback segment)
};

// The engaged units: battle_units[list * UNITS_PER_LIST + n], list 0 the attackers, list 1 the defenders.
constexpr uint8_t UNITS_PER_LIST{ 9 };
constexpr uint8_t ATTACKERS{ 0 };
constexpr uint8_t DEFENDERS{ 1 };

// The order kind a garrison holds its cell with; a unit under it when attacked tires least and loses no morale.
constexpr uint8_t HOLD_ORDER{ 3 };
// The order kind under which an attacked unit tires most and loses most morale.
constexpr uint8_t EXPOSED_ORDER{ 13 };

// The daily show of an attack and the far address of the routine it calls back (opeu_notice_view_finish).
constexpr uint16_t ATTACK_SHOW{ 486 };
constexpr uint16_t VIEW_FINISH_OFFSET{ 0x05fe };
constexpr uint16_t VIEW_FINISH_SEGMENT{ 0x33ad };

// The colour codes an attacker's side name is shown in.
constexpr uint16_t SIDE_COLOUR[]{ '6', '5' };

// side_control of a side a player commands (0: the computer).
constexpr uint16_t PLAYER_CONTROL{ 1 };

auto UnitAt(uint16_t list, uint8_t n) -> uint16_t {
  return static_cast<uint16_t>(list * UNITS_PER_LIST + n);
}

} // namespace

// Fills the garrison's element for the objective's item `slot`: up to a full element's quantity of the item's
// equipment (the item keeps the rest), morale by how full it is, training the objective's strength, fatigue by how
// far the strength is below its limit. Returns 1, or 0 for an empty item.
uint16_t opeu_notice_slot(uint8_t ds[65536], uint16_t objective, uint8_t slot) {
  auto& data{ Data(ds) };
  auto& city{ Record<Objective>(data, objective) };
  auto& item{ city.items[slot] };
  if (!item.quantity) return 0;
  auto& element{ data.battle_elements[slot] };
  element.next   = 0;
  element.type   = item.type;
  element.damage = 0;
  uint16_t const capacity{ data.request_capacities[data.element_types[item.type].category] };
  if (capacity > item.quantity)
    element.quantity = item.quantity;
  else
    element.quantity = capacity;
  item.quantity      -= element.quantity;
  element.morale      = static_cast<uint8_t>(opeu_min16(opeu_mul_div(element.quantity, 100, capacity), 99));
  element.training    = city.strength;
  element.experience  = static_cast<uint8_t>(100 - opeu_mul_div(city.strength, 100, city.strength_limit));
  element.flags       = Bits(ElementFlag::READY);
  if (city.flags & Bits(ObjectiveFlag::MARKED)) element.flags = Bits(ElementFlag::READY) | Bits(ElementFlag::SPOTTED);
  return 1;
}

// Builds the garrison of an objective on the battle cell from its items, with ammunition and fuel from its stocks up
// to the garrison's capacities, holding the cell; it becomes the first defender. Returns whether it has elements.
uint16_t opeu_notice_garrison(uint8_t ds[65536], HostEvent event) {
  ServiceLocals const locals{ 0x12 };
  auto&               data{ Data(ds) };
  uint16_t const      capacity_a{ locals.At(0xe) };
  uint16_t const      capacity_c{ locals.At(6) };
  uint16_t const      capacity_b{ locals.At(0) };
  uint16_t const      objective{ Call(ds, event, FIND_OBJECTIVE, data.battle_x, data.battle_y) };
  if (!objective) return 0;
  auto& garrison{ data.garrison };
  garrison.first = 0;
  uint8_t elements{ 0 };
  for (uint8_t slot = 0; slot < data.battle_elements.size(); slot++)
    if (opeu_notice_slot(ds, objective, slot)) {
      auto& element{ data.battle_elements[slot] };
      element.next   = garrison.first;
      garrison.first = Near(data, element);
      elements++;
    }
  if (!elements) return 0;
  auto& city{ Record<Objective>(data, objective) };
  garrison.x = data.battle_x;
  garrison.y = data.battle_y;
  Call(ds, event, UNIT_CAPACITIES, GARRISON_AT, capacity_a, capacity_b, capacity_c);
  garrison.resources[0]  = opeu_min16(Word(ds, capacity_a), city.resources[0]);
  garrison.resources[1]  = opeu_min16(Word(ds, capacity_b), city.resources[1]);
  city.resources[0]     -= garrison.resources[0];
  city.resources[1]     -= garrison.resources[1];
  auto& order{ garrison.orders[0] };
  order.target                                 = static_cast<uint16_t>(data.battle_x | data.battle_y << 8);
  order.kind                                   = HOLD_ORDER;
  order.progress                               = 0;
  garrison.flags                               = Bits(UnitFlag::READY) | Bits(UnitFlag::COMMUNICATIONS);
  Record<Element>(data, garrison.first).flags |= Bits(ElementFlag::SELECTED);
  data.battle_units[UnitAt(DEFENDERS, 0)]      = GARRISON_AT;
  return 1;
}

// Gathers the engaged units: the attacker and his side's units around the battle cell, the garrison, the defender
// and the other side's units around it.
void opeu_notice_collect(uint8_t ds[65536], HostEvent event) {
  ServiceLocals const locals{ 0xe };
  auto&               data{ Data(ds) };
  uint16_t const      x_at{ locals.At(4) };
  uint16_t const      y_at{ locals.At(0) };
  auto&               x{ ds[x_at] };
  auto&               y{ ds[y_at] };
  for (uint8_t n = 0; n < 2 * UNITS_PER_LIST; n++) data.battle_units[n] = 0;
  uint8_t  defenders{ static_cast<uint8_t>(opeu_notice_garrison(ds, event) ? 1 : 0) };
  uint16_t unit{ Call(ds, event, FIND_UNIT, data.battle_x, data.battle_y) };
  if (unit) data.battle_units[UnitAt(DEFENDERS, defenders++)] = unit;
  uint8_t attackers{ 1 };
  unit                                    = Call(ds, event, FIND_UNIT, data.battle_attacker_x, data.battle_attacker_y);
  data.battle_units[UnitAt(ATTACKERS, 0)] = unit;
  auto const side{ static_cast<uint8_t>(Call(ds, event, UNIT_SIDE, unit)) };
  for (uint8_t direction = 0; direction < 8; direction++) {
    x = data.battle_x;
    y = data.battle_y;
    if (!Call(ds, event, GRID_STEP, x_at, y_at, direction)) continue;
    if (!(unit = Call(ds, event, FIND_UNIT, x, y))) continue;
    if (static_cast<uint8_t>(Call(ds, event, UNIT_SIDE, unit)) == side) {
      if (x != data.battle_attacker_x || y != data.battle_attacker_y) data.battle_units[UnitAt(ATTACKERS, attackers++)] = unit;
    } else
      data.battle_units[UnitAt(DEFENDERS, defenders++)] = unit;
  }
}

// Records every engaged element with its unit and its equipment and quantity now; the rest of the records are
// cleared.
void opeu_notice_records_build(uint8_t ds[65536], HostEvent event) {
  auto&   data{ Data(ds) };
  uint8_t n{ 0 };
  for (uint8_t list = 0; list < 2; list++)
    for (uint8_t k = 0; k < UNITS_PER_LIST; k++) {
      uint16_t const unit{ data.battle_units[UnitAt(list, k)] };
      if (!unit) continue;
      for (uint16_t at = Record<Unit>(data, unit).first; at; at = Record<Element>(data, at).next) {
        auto& loss{ data.battle_losses[n++] };
        loss.unit    = Call(ds, event, ELEMENT_UNIT, at);
        loss.element = at;
        auto const& element{ Record<Element>(data, at) };
        loss.item.type     = element.type;
        loss.item.quantity = element.quantity;
        loss.item.extra    = element.damage;
      }
    }
  while (n < data.battle_losses.size()) data.battle_losses[n++].element = 0;
}

// Whether a player commands the side and one of the list's units (or the garrison's objective) may be commanded.
uint16_t opeu_notice_can_command(uint8_t ds[65536], HostEvent event, uint8_t list, uint8_t side) {
  auto& data{ Data(ds) };
  if (data.side_status[side] != SIDE_PLAYED) return 0;
  for (uint8_t k = 0; k < UNITS_PER_LIST; k++) {
    uint16_t const unit{ data.battle_units[UnitAt(list, k)] };
    if (!unit) continue;
    if (unit == GARRISON_AT) {
      if (Call(ds, event, OBJECTIVE_ELIGIBLE, Call(ds, event, FIND_OBJECTIVE, data.battle_x, data.battle_y))) return 1;
    } else if (Call(ds, event, UNIT_ELIGIBLE, unit))
      return 1;
  }
  return 0;
}

// Each side decides whether a leader takes command of the battle: the computer by a coin toss, a player when asked
// (only for a unit outside the computer's divisions). The battle is reported when a player's side could command.
void opeu_notice_decide(uint8_t ds[65536], HostEvent event) {
  ServiceLocals const     locals{ 0x72 };
  auto&                   data{ Data(ds) };
  uint16_t const          text{ locals.At(0) };
  std::array<uint16_t, 2> can_command{ 1, 1 };
  data.battle_command[0] = 0;
  data.battle_command[1] = 0;
  for (uint8_t list = 0; list < 2; list++)
    for (uint8_t k = 0; k < UNITS_PER_LIST; k++) {
      uint16_t const unit{ data.battle_units[UnitAt(list, k)] };
      uint16_t       objective;
      if (unit && unit != GARRISON_AT) {
        auto const side{ static_cast<uint8_t>(Call(ds, event, UNIT_SIDE, unit)) };
        if (!data.side_control[side]) {
          if (RandomBelow(ds, 100) < 50) data.battle_command[side] = 1;
          break;
        }
        if (data.side_control[side] == PLAYER_CONTROL && (can_command[side] = opeu_notice_can_command(ds, event, list, side)) != 0
            && !(Record<Group>(data, Call(ds, event, UNIT_GROUP, unit)).flags & Bits(GroupFlag::AI_MARK))) {
          Call(ds, event, FORMAT, text, offsetof(DataSegment, notice_command_text), data.side_names[side]);
          data.battle_command[side] = Call(ds, event, CONFIRM, text, Call(ds, event, SIDE_RECORD, side, 2));
          break;
        }
      } else if (unit == GARRISON_AT && (objective = Call(ds, event, FIND_OBJECTIVE, data.battle_x, data.battle_y)) != 0) {
        auto const side{ static_cast<uint8_t>(Call(ds, event, OBJECTIVE_SIDE, objective)) };
        if (!data.side_control[side]) {
          if (RandomBelow(ds, 100) < 50) data.battle_command[side] = 1;
          break;
        }
        if (data.side_control[side] == PLAYER_CONTROL && (can_command[side] = opeu_notice_can_command(ds, event, list, side)) != 0) {
          Call(ds, event, FORMAT, text, offsetof(DataSegment, notice_control_text), data.side_names[side]);
          data.battle_command[side] = Call(ds, event, CONFIRM, text, Call(ds, event, SIDE_RECORD, side, 2));
          break;
        }
      }
    }
  data.battle_report = (can_command[0] && data.side_status[0] == SIDE_PLAYED) || (can_command[1] && data.side_status[1] == SIDE_PLAYED) ? 1 : 0;
}

// How the battle is presented: 0 when it is not reported, else the battle_view setting, asking the player when the
// setting is to ask (0xff).
uint16_t opeu_notice_view_battle(uint8_t ds[65536], HostEvent event) {
  auto const& data{ Data(ds) };
  if (!data.battle_report) return 0;
  if (data.battle_view == 0xff) return Call(ds, event, CONFIRM, offsetof(DataSegment, notice_battle_text), Call(ds, event, SIDE_RECORD, (uint8_t)(data.side_control[0] == PLAYER_CONTROL ? 0 : 1), 2));
  return data.battle_view;
}

// The call-back of the attack's show.
void opeu_notice_view_finish(uint8_t ds[65536], HostEvent event) {
  Call(ds, event, PENDING_FINISH, 3);
}

// Tells the attacked unit or objective of the attack (naming a special attacker) and shows it to the attacked side;
// the attacked unit's elements tire and lose morale by its order: least when holding, most under EXPOSED_ORDER.
void opeu_notice_attack(uint8_t ds[65536], HostEvent event) {
  ServiceLocals const locals{ 0x80 };
  auto&               data{ Data(ds) };
  uint16_t const      text{ locals.At(0xc) };
  uint16_t const      name{ locals.At(0) };
  uint16_t const      attacker{ Call(ds, event, FIND_UNIT, data.battle_attacker_x, data.battle_attacker_y) };
  auto const          side{ static_cast<uint8_t>(Call(ds, event, UNIT_SIDE, attacker)) };
  uint16_t const      unit{ Call(ds, event, FIND_UNIT, data.battle_x, data.battle_y) };
  uint16_t const      objective{ Call(ds, event, FIND_OBJECTIVE, data.battle_x, data.battle_y) };
  if (Call(ds, event, UNIT_SPECIAL, attacker)) {
    uint16_t const side_name{ data.side_names[side] };
    Call(ds, event, FORMAT, text, offsetof(DataSegment, notice_attack_format), SIDE_COLOUR[side == 0 ? 0 : 1], side_name, Call(ds, event, UNIT_NAME, attacker, name));
  } else
    Call(ds, event, COPY_STRING, text, offsetof(DataSegment, notice_under_attack_text));
  if (unit) {
    Call(ds, event, UNIT_MESSAGE, unit, text);
    if (Call(ds, event, UNIT_ELIGIBLE, unit)) Call(ds, event, DAILY_SHOW, ATTACK_SHOW, opeu_opposite_side(side), VIEW_FINISH_OFFSET, VIEW_FINISH_SEGMENT);
  } else if (objective) {
    Call(ds, event, OBJECTIVE_REPORT, objective, text);
    if (Call(ds, event, OBJECTIVE_ELIGIBLE, objective)) Call(ds, event, DAILY_SHOW, ATTACK_SHOW, opeu_opposite_side(side), VIEW_FINISH_OFFSET, VIEW_FINISH_SEGMENT);
  }
  if (!unit) return;
  auto const& order{ Record<Unit>(data, unit).orders[0] };
  uint8_t     fatigue, morale_loss;
  if (order.kind == HOLD_ORDER && order.progress == 0) {
    fatigue     = 2;
    morale_loss = 0;
  } else if (order.kind == EXPOSED_ORDER && order.progress == 0) {
    fatigue     = 4;
    morale_loss = 6;
  } else {
    fatigue     = 3;
    morale_loss = 4;
  }
  for (uint16_t at = Record<Unit>(data, unit).first; at; at = Record<Element>(data, at).next) {
    auto& element{ Record<Element>(data, at) };
    opeu_add_byte(fatigue, 99, &element.experience);
    opeu_sub_byte(morale_loss, &element.morale);
  }
}

// The start of a battle.
void opeu_notice_begin(uint8_t ds[65536], HostEvent event) {
  auto& data{ Data(ds) };
  opeu_notice_attack(ds, event);
  opeu_notice_collect(ds, event);
  data.presentation_mode = opeu_notice_view_battle(ds, event);
  opeu_notice_decide(ds, event);
  opeu_notice_records_build(ds, event);
}

uint16_t svc_notice(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case FIND_OBJECTIVE:     r = opeu_find_objective_at(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case FIND_UNIT:          r = opeu_find_unit_at(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case UNIT_CAPACITIES:    StoreUnitCapacities(ds, Arg(args, count, 0), { Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3) }); break;
  case UNIT_SIDE:          r = opeu_unit_side(Arg(args, count, 0)); break;
  case GRID_STEP:          r = opeu_grid_step(ds + Arg(args, count, 0), ds + Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case ELEMENT_UNIT:       r = opeu_find_element_unit(ds, Arg(args, count, 0)); break;
  case UNIT_ELIGIBLE:      r = opeu_unit_selection_eligible(ds, Arg(args, count, 0)); break;
  case OBJECTIVE_ELIGIBLE: r = opeu_objective_eligible(ds, Arg(args, count, 0)); break;
  case UNIT_GROUP:         r = opeu_unit_group(Arg(args, count, 0)); break;
  case OBJECTIVE_SIDE:     r = opeu_objective_side(ds, Arg(args, count, 0)); break;
  case FORMAT:             opeu_request_format(ds, { args, count }); break;
  case SIDE_RECORD:        r = opeu_find_side_record(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case CONFIRM:            r = opeu_confirm_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case PENDING_FINISH:     opeu_pending_finish(ds, svc_waitkey, (uint8_t)Arg(args, count, 0)); break;
  case UNIT_SPECIAL:       r = opeu_unit_special(ds, Arg(args, count, 0)); break;
  case UNIT_NAME:          r = opeu_unit_name(ds, svc_unitname, Arg(args, count, 0), Arg(args, count, 1)); break;
  case COPY_STRING:        r = opeu_string_copy(ds, Arg(args, count, 0), Arg(args, count, 1)); break;
  case UNIT_MESSAGE:       opeu_unit_report(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case OBJECTIVE_REPORT:   opeu_objective_report(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case DAILY_SHOW: opeu_daily_show(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint32_t)Arg(args, count, 3) << 16 | Arg(args, count, 2)); break;
  default:         port_unhandled("notice", id, count);
  }
  return r;
}

auto port_routine_opeu_notice_view_finish(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_notice_view_finish(ds, svc_notice);
  return 0;
}
