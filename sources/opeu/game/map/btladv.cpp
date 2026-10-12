// The battle element's round: its advance off the field (onto the strategic map or out of the battle), the
// withdrawal and hold steps, and the dispatch on its state.
#include "map/btladv.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "core/numeric.hpp"
#include "core/random.hpp"
#include "map/btlcell.hpp"
#include "map/btlicon.hpp"
#include "map/btllist.hpp"
#include "map/btlmove.hpp"
#include "map/btlunit.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "types.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// The strategic map mark of the unit's side.
[[nodiscard]] auto SideMark(uint8_t* ds, BattleEvent event, uint16_t unit) -> uint8_t {
  return Bits(opeu_battle_unit_side(ds, event, unit) ? MapCell::MARK_1 : MapCell::MARK_0);
}

// Takes percent of the value away.
void Reduce(uint16_t& value, uint16_t percent) {
  uint16_t left{ value };
  opeu_sub_word(opeu_mul_div(left, percent, 100), &left);
  value = left;
}

[[nodiscard]] auto UnitDirection(uint8_t* ds, BattleEvent event, uint16_t unit) -> uint8_t {
  return static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_UNIT_DIRECTION, unit));
}

[[nodiscard]] auto Strength(uint8_t* ds, BattleEvent event, uint16_t element) -> uint8_t {
  return static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_ELEMENT_STRENGTH, element));
}
} // namespace

void opeu_battle_exit_to_map(uint8_t* ds, BattleEvent event, uint16_t element, uint8_t x, uint8_t y) {
  auto&      data{ Data(ds) };
  auto const unit{ opeu_battle_element_unit(ds, event, element) };
  Call(ds, event, BattleRoutine::BATTLE_LOSS_CHECK, element, 0);
  auto const mark{ SideMark(ds, event, unit) };
  MapCellAt(data, x, y)         |= mark;
  data.battle_exit_direction[0]  = 0xff;
  if (opeu_battle_unit_active(ds, event, unit)) {
    data.battle_exit_direction[0] = 0xff;
    for (auto& other : Elements(data, Record<Unit>(data, unit)))
      if (opeu_battle_state(ds, event, Near(data, other)) == Bits(BattleState::WITHDRAW)) opeu_battle_update_element(ds, event, Near(data, other));
  }
}

void opeu_battle_element_withdraw(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto&      data{ Data(ds) };
  auto const goal_x{ PositionOf(data, element).goal_x };
  auto const goal_y{ PositionOf(data, element).goal_y };
  auto const unit{ opeu_battle_element_unit(ds, event, element) };
  if (opeu_battle_element_visible(ds, event, element)) opeu_battle_draw_cell(ds, event, PositionOf(data, element).x, PositionOf(data, element).y);
  if (UnitDirection(ds, event, unit) == BATTLE_NOT_ADJACENT) {
    auto const direction{ static_cast<uint8_t>((BattleCellAt(data, goal_x, goal_y) >> std::to_underlying(BattleCell::EXIT_SHIFT)) - 1) };
    uint8_t    x{ data.battle_x };
    uint8_t    y{ data.battle_y };
    opeu_grid_step(&x, &y, direction);
    if ((MapCellAt(data, x, y) & Bits(MapCell::MARK_MASK)) && !(SideMark(ds, event, unit) & MapCellAt(data, x, y))) {
      opeu_battle_exit_to_map(ds, event, element, x, y);
      return;
    }
  }
  opeu_battle_clear_cell(ds, event, PositionOf(data, element).x, PositionOf(data, element).y);
  PositionOf(data, element).x |= BATTLE_LEFT;
  PositionOf(data, element).y |= BATTLE_LEFT;
  uint8_t active{ 0 };
  for (auto& other : Elements(data, Record<Unit>(data, unit)))
    if (opeu_battle_element_active(ds, event, Near(data, other))) ++active;
  if (active) return;
  auto const garrison{ Near(data, data.battle_garrison) };
  if (UnitDirection(ds, event, unit) == BATTLE_NOT_ADJACENT && unit != garrison) data.battle_direction = data.battle_exit_direction[0];
  if (unit == garrison) {
    auto&   objective{ Record<Objective>(data, opeu_find_objective_at(ds, data.battle_x, data.battle_y)) };
    uint8_t gone{ 0 };
    for (auto& other : Elements(data, Record<Unit>(data, unit)))
      if ((other.flags & Bits(ElementFlag::READY)) && (PositionOf(data, Near(data, other)).x & BATTLE_LEFT)) ++gone;
    auto const percent{ opeu_rand_bounded(static_cast<uint16_t>(gone * 10u + 41)) };
    Reduce(objective.resources[1], percent);
    Reduce(objective.resources[2], percent);
    Reduce(objective.resources[0], percent);
    for (auto& item : objective.items)
      if (item.quantity) item.quantity = 0;
  }
  opeu_battle_remove_list(ds, event, unit);
}

void opeu_battle_withdraw_step(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto const order{ opeu_battle_order(ds, event, element) };
  if (static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_EXECUTE_ORDER, order, element)) != Bits(BattleState::WITHDRAW)) return;
  opeu_battle_element_withdraw(ds, event, element);
}

void opeu_battle_hold_step(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto& data{ Data(ds) };
  auto  order{ opeu_battle_order(ds, event, element) };
  if (opeu_battle_level_allowed(ds, event, element)) {
    if (auto const target{ opeu_battle_best_target(ds, event, element) }) {
      if (Strength(ds, event, element) <= order) {
        Call(ds, event, BattleRoutine::BATTLE_ATTACK_OR_HOLD, target, element);
        return;
      }
      if (data.element_types[Record<Element>(data, element).type].category) {
        auto const facing{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_ELEMENT_FACING, target, element)) };
        Call(ds, event, BattleRoutine::BATTLE_ANIMATE_FACING, facing, element);
      }
    }
  }
  if (opeu_rand_bounded(10) < 2) Call(ds, event, BattleRoutine::BATTLE_SPOT_ENEMIES, element);
  if (Strength(ds, event, element) > order) opeu_battle_set_order(ds, event, element, ++order);
}

void opeu_battle_element_step(uint8_t* ds, BattleEvent event, uint16_t element) {
  switch (static_cast<BattleState>(opeu_battle_state(ds, event, element))) {
  case BattleState::MOVE:     Call(ds, event, BattleRoutine::BATTLE_ADVANCE, element); break;
  case BattleState::ATTACK:   Call(ds, event, BattleRoutine::BATTLE_CONTINUE_ORDER, element); break;
  case BattleState::BLITZ:    Call(ds, event, BattleRoutine::BATTLE_SEEK_TARGET, element); break;
  case BattleState::DEFEND:   opeu_battle_hold_step(ds, event, element); break;
  case BattleState::WITHDRAW: opeu_battle_withdraw_step(ds, event, element); break;
  default:                    break;
  }
}

uint16_t svc_btladv(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(ds, address, args, count);
}

auto port_routine_opeu_battle_element_step(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_element_step(ds, svc_btladv, Arg(args, count, 0));
  return 0;
}
