// The battle element's round: its advance off the field (onto the strategic map or out of the battle), the
// withdrawal and hold steps, and the dispatch on its state.
#include "map/btladv.hpp"
#include "common/battle-calls.hpp"
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

#include "common/game.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// The strategic map mark of the unit's side.
[[nodiscard]] auto SideMark(Game& game, BattleEvent event, uint16_t unit) -> uint8_t {
  return Bits(opeu_battle_unit_side(game, event, unit) ? MapCell::MARK_1 : MapCell::MARK_0);
}

} // namespace

void opeu_battle_exit_to_map(Game& game, BattleEvent event, uint16_t element, uint8_t x, uint8_t y) {
  auto&      data{ Data(game) };
  auto const unit{ opeu_battle_element_unit(game, event, element) };
  Call(game, event, BattleRoutine::BATTLE_LOSS_CHECK, element, 0);
  auto const mark{ SideMark(game, event, unit) };
  MapCellAt(data, x, y)         |= mark;
  data.battle_exit_direction[0]  = 0xff;
  if (opeu_battle_unit_active(game, event, unit)) {
    data.battle_exit_direction[0] = 0xff;
    for (auto& other : Elements(data, Record<Unit>(data, unit)))
      if (opeu_battle_state(game, event, Near(data, other)) == Bits(BattleState::WITHDRAW)) opeu_battle_update_element(game, event, Near(data, other));
  }
}

void opeu_battle_element_withdraw(Game& game, BattleEvent event, uint16_t element) {
  auto&      data{ Data(game) };
  auto const goal_x{ PositionOf(data, element).goal_x };
  auto const goal_y{ PositionOf(data, element).goal_y };
  auto const unit{ opeu_battle_element_unit(game, event, element) };
  if (opeu_battle_element_visible(game, event, element)) opeu_battle_draw_cell(game, event, PositionOf(data, element).x, PositionOf(data, element).y);
  if (UnitDirection(game, event, unit) == BATTLE_NOT_ADJACENT) {
    auto const direction{ static_cast<uint8_t>((BattleCellAt(data, goal_x, goal_y) >> std::to_underlying(BattleCell::EXIT_SHIFT)) - 1) };
    uint8_t    x{ data.battle_x };
    uint8_t    y{ data.battle_y };
    opeu_grid_step(&x, &y, direction);
    if ((MapCellAt(data, x, y) & Bits(MapCell::MARK_MASK)) && !(SideMark(game, event, unit) & MapCellAt(data, x, y))) {
      opeu_battle_exit_to_map(game, event, element, x, y);
      return;
    }
  }
  opeu_battle_clear_cell(game, event, PositionOf(data, element).x, PositionOf(data, element).y);
  PositionOf(data, element).x |= BATTLE_LEFT;
  PositionOf(data, element).y |= BATTLE_LEFT;
  uint8_t active{ 0 };
  for (auto& other : Elements(data, Record<Unit>(data, unit)))
    if (opeu_battle_element_active(game, event, Near(data, other))) ++active;
  if (active) return;
  auto const garrison{ Near(data, data.battle_garrison) };
  if (UnitDirection(game, event, unit) == BATTLE_NOT_ADJACENT && unit != garrison) data.battle_direction = data.battle_exit_direction[0];
  if (unit == garrison) {
    auto&   objective{ Record<Objective>(data, opeu_find_objective_at(game, data.battle_x, data.battle_y)) };
    uint8_t gone{ 0 };
    for (auto& other : Elements(data, Record<Unit>(data, unit)))
      if ((other.flags & Bits(ElementFlag::READY)) && (PositionOf(data, Near(data, other)).x & BATTLE_LEFT)) ++gone;
    auto const percent{ opeu_rand_bounded(static_cast<uint16_t>(gone * 10u + 41)) };
    AbandonObjectiveStores(objective, percent);
  }
  opeu_battle_remove_list(game, event, unit);
}

void opeu_battle_withdraw_step(Game& game, BattleEvent event, uint16_t element) {
  auto const order{ opeu_battle_order(game, event, element) };
  if (static_cast<uint8_t>(Call(game, event, BattleRoutine::BATTLE_EXECUTE_ORDER, order, element)) != Bits(BattleState::WITHDRAW)) return;
  opeu_battle_element_withdraw(game, event, element);
}

void opeu_battle_hold_step(Game& game, BattleEvent event, uint16_t element) {
  auto& data{ Data(game) };
  auto  order{ opeu_battle_order(game, event, element) };
  if (opeu_battle_level_allowed(game, event, element)) {
    if (auto const target{ opeu_battle_best_target(game, event, element) }) {
      if (ElementStrength(game, event, element) <= order) {
        Call(game, event, BattleRoutine::BATTLE_ATTACK_OR_HOLD, target, element);
        return;
      }
      if (ElementTypeOf(data, element).category) {
        auto const facing{ static_cast<uint8_t>(Call(game, event, BattleRoutine::BATTLE_ELEMENT_FACING, target, element)) };
        Call(game, event, BattleRoutine::BATTLE_ANIMATE_FACING, facing, element);
      }
    }
  }
  if (opeu_rand_bounded(10) < 2) Call(game, event, BattleRoutine::BATTLE_SPOT_ENEMIES, element);
  if (ElementStrength(game, event, element) > order) opeu_battle_set_order(game, event, element, ++order);
}

void opeu_battle_element_step(Game& game, BattleEvent event, uint16_t element) {
  switch (static_cast<BattleState>(opeu_battle_state(game, event, element))) {
  case BattleState::MOVE:     Call(game, event, BattleRoutine::BATTLE_ADVANCE, element); break;
  case BattleState::ATTACK:   Call(game, event, BattleRoutine::BATTLE_CONTINUE_ORDER, element); break;
  case BattleState::BLITZ:    Call(game, event, BattleRoutine::BATTLE_SEEK_TARGET, element); break;
  case BattleState::DEFEND:   opeu_battle_hold_step(game, event, element); break;
  case BattleState::WITHDRAW: opeu_battle_withdraw_step(game, event, element); break;
  default:                    break;
  }
}

uint16_t svc_btladv(Game& game, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(game, address, args, count);
}

auto port_routine_opeu_battle_element_step(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_element_step(game, svc_btladv, Arg(args, count, 0));
  return 0;
}
