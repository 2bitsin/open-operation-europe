// The computer's battle turn: each unit's plan and target, each element's action, and the select service that
// runs them (for the selected element alone, or for every computer unit of both sides).
//
// The plan byte of a list slot holds the profile in the low nibble and the plan in the high nibble; the target slot
// holds the unit it moves against.
#include "map/btlaitrn.hpp"
#include "common/battle-calls.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btlaiact.hpp"
#include "map/btlaient.hpp"
#include "map/btlaimov.hpp"
#include "map/btlaipwr.hpp"
#include "map/btlaiunt.hpp"
#include "map/btlcalc.hpp"
#include "map/btllist.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

[[nodiscard]] auto UnitAt(DataSegment& data, uint16_t unit, uint8_t x, uint8_t y) -> bool {
  auto const& record{ Record<Unit>(data, unit) };
  return record.x == x && record.y == y;
}
} // namespace

void opeu_battle_ai_plan_unit(Game& game, uint16_t element) {
  auto&      data{ Data(game) };
  auto const mode{ ElementTypeOf(data, element).gun_range };
  auto const target{ opeu_battle_ai_unit_target(game, element) };
  if (opeu_battle_ai_choose_plan(game, target) < 3 && mode <= RANGED_MODE && target)
    opeu_battle_ai_attack_near(game, element, target);
  else
    opeu_battle_ai_hold(game, element);
}

void opeu_battle_ai_enter_or_hold(Game& game, uint16_t element) {
  if (!opeu_battle_ai_enter(game, element)) opeu_battle_ai_hold(game, element);
}

void opeu_battle_ai_element(Game& game, uint16_t element) {
  auto&       data{ Data(game) };
  auto const  unit{ opeu_battle_element_unit(game, element) };
  auto const& record{ Record<Unit>(data, unit) };
  // A heavy element of a unit out of fuel stays; a unit out of ammunition, or set to leave, makes for the edge.
  if (ElementTypeOf(data, element).fuel_capacity && record.resources[1] == 0) {
    opeu_battle_ai_hold(game, element);
    return;
  }
  if (record.resources[0] == 0 || opeu_battle_ai_plan_low(game, element) == 4 || opeu_battle_ai_plan_high(game, element) == 1) {
    opeu_battle_ai_enter_or_hold(game, element);
    return;
  }
  if (unit == Near(data, data.battle_garrison))
    opeu_battle_ai_plan_unit(game, element);
  else
    opeu_battle_ai_think(game, element);
}

void opeu_battle_ai_assign_targets(Game& game, uint8_t side, uint8_t index) {
  auto& data{ Data(game) };
  auto& slot{ data.battle_ai_targets[side * BATTLE_LIST_LENGTH + index] };
  if (slot && opeu_battle_unit_active(game, slot)) return;
  slot = 0;
  auto const unit{ BattleList(data, side, index) };
  auto const direction{ UnitDirection(game, unit) };
  (void)opeu_battle_unit_side(game, unit);
  if (direction == BATTLE_NOT_ADJACENT) {
    auto const target{ BattleList(data, 0, 0) };
    if (UnitAt(data, target, data.battle_attacker_x, data.battle_attacker_y)) slot = target;
  } else if (side == 1) {
    for (uint8_t i{ 0 }; i < BATTLE_LIST_LENGTH; ++i) {
      auto const target{ BattleList(data, 0, i) };
      if (!target) continue;
      auto const other{ UnitDirection(game, target) };
      if (data.battle_ai_pairs[direction * 2u] == other || data.battle_ai_pairs[direction * 2u + 1] == other) slot = target;
    }
  } else {
    auto const first{ BattleList(data, 1, 0) };
    auto const second{ BattleList(data, 1, 1) };
    if (UnitAt(data, first, data.battle_x, data.battle_y))
      slot = first;
    else if (UnitAt(data, second, data.battle_x, data.battle_y))
      slot = second;
  }
  if (slot == 0) slot = BattleList(data, side == 0 ? 1 : 0, 0);
}

void opeu_battle_ai_plan_side(Game& game, uint16_t unit) {
  auto&      data{ Data(game) };
  auto const plan{ Record<Unit>(data, unit).resources[0] == 0 ? uint8_t{ 1 } : opeu_battle_ai_choose_plan(game, unit) };
  auto const profile{ opeu_battle_ai_profile(game, unit, 1) };
  data.battle_ai_plans[data.battle_ai_side * BATTLE_LIST_LENGTH + data.battle_ai_index] = static_cast<uint8_t>(profile | plan << 4);
  opeu_battle_ai_assign_targets(game, data.battle_ai_side, data.battle_ai_index);
  for (auto& element : Elements(data, Record<Unit>(data, unit)))
    if (opeu_battle_element_active(game, Near(data, element))) opeu_battle_ai_element(game, Near(data, element));
}

void opeu_battle_select_service(Game& game) {
  auto& data{ Data(game) };
  if (auto const selected{ data.battle_selected }) {
    auto const unit{ opeu_battle_element_unit(game, selected) };
    auto const side{ opeu_battle_unit_side(game, unit) };
    data.battle_ai_side = static_cast<uint8_t>(opeu_battle_unit_side(game, BattleList(data, 0, 0)) == side ? 0 : 1);
    for (uint8_t i{ 0 }; i < BATTLE_LIST_LENGTH; ++i)
      if (BattleList(data, data.battle_ai_side, i) == unit) {
        data.battle_ai_index = i;
        break;
      }
    opeu_battle_ai_element(game, data.battle_selected);
    data.battle_selected = 0;
    return;
  }
  for (uint8_t side{ 0 }; side < 2; ++side) {
    data.battle_ai_side = side;
    for (uint8_t i{ 0 }; i < BATTLE_LIST_LENGTH; ++i) {
      data.battle_ai_index = i;
      auto const unit{ BattleList(data, side, i) };
      if (unit && !opeu_battle_unit_controlled(game, unit) && opeu_battle_level_allowed(game, Record<Unit>(data, unit).first)) opeu_battle_ai_plan_side(game, unit);
    }
  }
}
