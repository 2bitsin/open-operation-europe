// The start of a battle: the attacked unit or objective is told and worn, both sides' engaged units are gathered
// (a besieged objective fights as a garrison pseudo-unit built from its equipment), each side decides whether a
// leader takes command and whether the battle is reported, and every engaged element is recorded for the loss
// report at its end (core/notend).
#include "core/notice.hpp"
#include "common/events.hpp"
#include "core/grid-neighbours.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/sideutil.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/elemfind.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "core/strings.hpp"
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

// The engaged units: battle_units[list * UNITS_PER_LIST + n], list 0 the attackers, list 1 the defenders.
constexpr uint8_t UNITS_PER_LIST{ 9 };
constexpr uint8_t ATTACKERS{ 0 };
constexpr uint8_t DEFENDERS{ 1 };

// The order kind a garrison holds its cell with; a unit under it when attacked tires least and loses no morale.
constexpr uint8_t HOLD_ORDER{ 3 };
// The order kind under which an attacked unit tires most and loses most morale.
constexpr uint8_t EXPOSED_ORDER{ 13 };

// The daily show of an attack.
constexpr uint16_t ATTACK_SHOW{ 486 };

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
uint16_t opeu_notice_slot(Game& game, uint16_t objective, uint8_t slot) {
  auto& data{ Data(game) };
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
uint16_t opeu_notice_garrison(Game& game) {
  auto&          data{ Data(game) };
  uint16_t const objective{ opeu_find_objective_at(game, data.battle_x, data.battle_y) };
  if (!objective) return 0;
  auto& garrison{ data.garrison };
  garrison.first = 0;
  uint8_t elements{ 0 };
  for (uint8_t slot = 0; slot < data.battle_elements.size(); slot++)
    if (opeu_notice_slot(game, objective, slot)) {
      auto& element{ data.battle_elements[slot] };
      element.next   = garrison.first;
      garrison.first = Near(data, element);
      elements++;
    }
  if (!elements) return 0;
  auto& city{ Record<Objective>(data, objective) };
  garrison.x = data.battle_x;
  garrison.y = data.battle_y;
  auto const capacities{ UnitCapacities(data, GARRISON_AT) };
  garrison.resources[0]  = opeu_min16(capacities.ammunition, city.resources[0]);
  garrison.resources[1]  = opeu_min16(capacities.fuel, city.resources[1]);
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
void opeu_notice_collect(Game& game) {
  auto& data{ Data(game) };
  for (uint8_t n = 0; n < 2 * UNITS_PER_LIST; n++) data.battle_units[n] = 0;
  uint8_t  defenders{ static_cast<uint8_t>(opeu_notice_garrison(game) ? 1 : 0) };
  uint16_t unit{ opeu_find_unit_at(game, data.battle_x, data.battle_y) };
  if (unit) data.battle_units[UnitAt(DEFENDERS, defenders++)] = unit;
  uint8_t attackers{ 1 };
  unit                                    = opeu_find_unit_at(game, data.battle_attacker_x, data.battle_attacker_y);
  data.battle_units[UnitAt(ATTACKERS, 0)] = unit;
  auto const side{ opeu_unit_side(unit) };
  for (auto const [x, y] : CellsAround({ data.battle_x, data.battle_y })) {
    if (!(unit = opeu_find_unit_at(game, x, y))) continue;
    if (opeu_unit_side(unit) == side) {
      if (x != data.battle_attacker_x || y != data.battle_attacker_y) data.battle_units[UnitAt(ATTACKERS, attackers++)] = unit;
    } else
      data.battle_units[UnitAt(DEFENDERS, defenders++)] = unit;
  }
}

// Records every engaged element with its unit and its equipment and quantity now; the rest of the records are
// cleared.
void opeu_notice_records_build(Game& game) {
  auto&   data{ Data(game) };
  uint8_t n{ 0 };
  for (uint8_t list = 0; list < 2; list++)
    for (uint8_t k = 0; k < UNITS_PER_LIST; k++) {
      uint16_t const unit{ data.battle_units[UnitAt(list, k)] };
      if (!unit) continue;
      for (uint16_t at = Record<Unit>(data, unit).first; at; at = Record<Element>(data, at).next) {
        auto& loss{ data.battle_losses[n++] };
        loss.unit    = opeu_find_element_unit(game, at);
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
uint16_t opeu_notice_can_command(Game& game, uint8_t list, uint8_t side) {
  auto& data{ Data(game) };
  if (data.side_status[side] != SIDE_PLAYED) return 0;
  for (uint8_t k = 0; k < UNITS_PER_LIST; k++) {
    uint16_t const unit{ data.battle_units[UnitAt(list, k)] };
    if (!unit) continue;
    if (unit == GARRISON_AT) {
      if (opeu_objective_eligible(game, opeu_find_objective_at(game, data.battle_x, data.battle_y))) return 1;
    } else if (opeu_unit_selection_eligible(game, unit))
      return 1;
  }
  return 0;
}

// Each side decides whether a leader takes command of the battle: the computer by a coin toss, a player when asked
// (only for a unit outside the computer's divisions). The battle is reported when a player's side could command.
void opeu_notice_decide(Game& game) {
  TextBuffer<0x72>        locals;
  auto&                   data{ Data(game) };
  auto const              text{ locals.At(0) };
  std::array<uint16_t, 2> can_command{ 1, 1 };
  data.battle_command[0] = 0;
  data.battle_command[1] = 0;
  for (uint8_t list = 0; list < 2; list++)
    for (uint8_t k = 0; k < UNITS_PER_LIST; k++) {
      uint16_t const unit{ data.battle_units[UnitAt(list, k)] };
      uint16_t       objective;
      if (unit && unit != GARRISON_AT) {
        auto const side{ opeu_unit_side(unit) };
        if (!data.side_control[side]) {
          if (game.State().RandomBounded(100) < 50) data.battle_command[side] = 1;
          break;
        }
        if (data.side_control[side] == PLAYER_CONTROL && (can_command[side] = opeu_notice_can_command(game, list, side)) != 0
            && !(Record<Group>(data, opeu_unit_group(unit)).flags & Bits(GroupFlag::AI_MARK))) {
          opeu_request_format(game, text, std::array<FormatArgument, 2>{ offsetof(DataSegment, notice_command_text), data.side_names[side] });
          data.battle_command[side] = opeu_confirm_message(game, text, opeu_find_side_record(game, side, 2));
          break;
        }
      } else if (unit == GARRISON_AT && (objective = opeu_find_objective_at(game, data.battle_x, data.battle_y)) != 0) {
        auto const side{ opeu_objective_side(game, objective) };
        if (!data.side_control[side]) {
          if (game.State().RandomBounded(100) < 50) data.battle_command[side] = 1;
          break;
        }
        if (data.side_control[side] == PLAYER_CONTROL && (can_command[side] = opeu_notice_can_command(game, list, side)) != 0) {
          opeu_request_format(game, text, std::array<FormatArgument, 2>{ offsetof(DataSegment, notice_control_text), data.side_names[side] });
          data.battle_command[side] = opeu_confirm_message(game, text, opeu_find_side_record(game, side, 2));
          break;
        }
      }
    }
  data.battle_report = (can_command[0] && data.side_status[0] == SIDE_PLAYED) || (can_command[1] && data.side_status[1] == SIDE_PLAYED) ? 1 : 0;
}

// How the battle is presented: 0 when it is not reported, else the battle_view setting, asking the player when the
// setting is to ask (0xff).
uint16_t opeu_notice_view_battle(Game& game) {
  auto const& data{ Data(game) };
  if (!data.battle_report) return 0;
  if (data.battle_view == 0xff) return opeu_confirm_message(game, offsetof(DataSegment, notice_battle_text), opeu_find_side_record(game, data.side_control[0] == PLAYER_CONTROL ? 0 : 1, 2));
  return data.battle_view;
}

// The call-back of the attack's show.
void opeu_notice_view_finish(Game& game) {
  opeu_pending_finish(game, 3);
}

// Tells the attacked unit or objective of the attack (naming a special attacker) and shows it to the attacked side;
// the attacked unit's elements tire and lose morale by its order: least when holding, most under EXPOSED_ORDER.
void opeu_notice_attack(Game& game) {
  TextBuffer<0x80> locals;
  auto&            data{ Data(game) };
  auto const       text{ locals.At(0xc) };
  auto const       name{ locals.At(0) };
  uint16_t const   attacker{ opeu_find_unit_at(game, data.battle_attacker_x, data.battle_attacker_y) };
  auto const       side{ opeu_unit_side(attacker) };
  uint16_t const   unit{ opeu_find_unit_at(game, data.battle_x, data.battle_y) };
  uint16_t const   objective{ opeu_find_objective_at(game, data.battle_x, data.battle_y) };
  if (opeu_unit_special(game, attacker)) {
    uint16_t const side_name{ data.side_names[side] };
    opeu_request_format(game, text, std::array<FormatArgument, 4>{ offsetof(DataSegment, notice_attack_format), SIDE_COLOUR[side == 0 ? 0 : 1], side_name, opeu_unit_name(game, attacker, name) });
  } else
    opeu_string_copy(game, text, offsetof(DataSegment, notice_under_attack_text));
  if (unit) {
    opeu_unit_report(game, unit, text);
    if (opeu_unit_selection_eligible(game, unit)) opeu_daily_show(game, ATTACK_SHOW, opeu_opposite_side(side), Presenter::NOTICE_VIEW);
  } else if (objective) {
    opeu_objective_report(game, objective, text);
    if (opeu_objective_eligible(game, objective)) opeu_daily_show(game, ATTACK_SHOW, opeu_opposite_side(side), Presenter::NOTICE_VIEW);
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
    element.experience = AddByte(element.experience, fatigue, 99);
    element.morale     = SubByte(element.morale, morale_loss);
  }
}

// The start of a battle.
void opeu_notice_begin(Game& game) {
  auto& data{ Data(game) };
  opeu_notice_attack(game);
  opeu_notice_collect(game);
  data.presentation_mode = opeu_notice_view_battle(game);
  opeu_notice_decide(game);
  opeu_notice_records_build(game);
}
