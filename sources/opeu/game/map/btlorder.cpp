// Battle order execution: advancing, attacking and holding.
#include "map/btlorder.hpp"
#include "common/battle-calls.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btlcalc.hpp"
#include "map/btldir.hpp"
#include "map/btlengag.hpp"
#include "map/btllist.hpp"
#include "map/btlloss.hpp"
#include "map/btlmove.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t NO_FACING{ 0xff };

// opeu_battle_execute_order's results.
constexpr uint8_t STEPPED{ 0 };
constexpr uint8_t WAITING{ 1 };
constexpr uint8_t UPDATED{ 2 };
constexpr uint8_t ARRIVED{ 3 };

// BATTLE_NEXT_STEP's verdicts and BATTLE_STEP_INTO's.
constexpr uint8_t STEP_WAIT{ 1 };
constexpr uint8_t STEP_SEARCH{ 2 };
constexpr uint8_t STEP_STUCK{ 3 };
constexpr uint8_t INTO_BLOCKED{ 1 };
constexpr uint8_t INTO_LOST{ 2 };

[[nodiscard]] auto Is(uint8_t state, BattleState kind) -> bool {
  return state == std::to_underlying(kind);
}

[[nodiscard]] auto Position(Game& game, uint16_t element) -> BattlePosition& {
  return PositionOf(Data(game), element);
}

// Vehicles turn to face where they fire or go; infantry does not.
[[nodiscard]] auto Turns(Game& game, uint16_t element) -> bool {
  auto const& data{ Data(game) };
  return ElementTypeOf(data, element).category != 0;
}

void Face(Game& game, uint16_t element, uint8_t facing) {
  opeu_battle_animate_facing(game, element, static_cast<uint8_t>(facing));
}

void FaceTarget(Game& game, uint16_t element, uint16_t target) {
  Face(game, element, static_cast<uint8_t>(opeu_battle_element_facing(game, element, target)));
}

void FaceCell(Game& game, uint16_t element, uint8_t x, uint8_t y) {
  auto const& position{ Position(game, element) };
  Face(game, element, static_cast<uint8_t>(opeu_battle_cell_facing(game, static_cast<uint8_t>(position.x), static_cast<uint8_t>(position.y), static_cast<uint8_t>(x), static_cast<uint8_t>(y))));
}
} // namespace

// Moves the element a step toward its goal or its target. order counts the element's waits this round. The cell it
// heads for is two DS words: the path step services receive their addresses.
uint8_t opeu_battle_execute_order(Game& game, uint16_t element, uint8_t order) {
  ServiceLocals const locals{ 4 };
  auto const          x_at{ locals.At(0) };
  auto const          y_at{ locals.At(2) };
  auto&               x{ Byte(game, x_at) };
  auto&               y{ Byte(game, y_at) };
  auto const          update{ [&] {
    opeu_battle_update_element(game, element);
    return UPDATED;
  } };
  auto const wait{ [&] {
    opeu_battle_set_order(game, element, ++order);
    return WAITING;
  } };
  auto const head_for_goal{ [&] {
    auto const& position{ Position(game, element) };
    x = position.goal_x;
    y = position.goal_y;
  } };
  auto const head_for_target{ [&](uint16_t target) {
    auto const& position{ Position(game, target) };
    x = position.x;
    y = position.y;
  } };
  bool retry{ true };
  for (;;) {
    auto const state{ opeu_battle_state(game, element) };
    if (Is(state, BattleState::MOVE) || Is(state, BattleState::WITHDRAW)) {
      head_for_goal();
    } else if (Is(state, BattleState::BLITZ)) {
      if (!opeu_battle_engaged(game, element)) {
        head_for_goal();
      } else {
        auto const target{ Position(game, element).target };
        if (!opeu_battle_element_active(game, target)) return update();
        head_for_target(target);
      }
    } else if (Is(state, BattleState::ATTACK)) {
      if (!opeu_battle_engaged(game, element))
        head_for_goal();
      else
        head_for_target(Position(game, element).target);
    }
    if (opeu_battle_facing(game, element) == NO_FACING) {
      switch (static_cast<uint8_t>(opeu_battle_next_step(game, element, x_at, y_at))) {
      case STEP_WAIT: return wait();
      case STEP_SEARCH:
        if (!opeu_battle_element_path_step(game, element, x_at, y_at)) return WAITING;
        break;
      case STEP_STUCK: return update();
      default:         break;
      }
      opeu_battle_set_strength(game, element, x, y);
      opeu_battle_set_facing(game, element, static_cast<uint8_t>(opeu_battle_move_cost(game, svc_btlcalc, static_cast<uint8_t>(x), static_cast<uint8_t>(y), element)));
    }
    if (opeu_battle_facing(game, element) > order) return wait();
    x = opeu_battle_strength_low(game, element);
    y = opeu_battle_strength_high(game, element);
    auto const into{ static_cast<uint8_t>(opeu_battle_step_into(game, element, static_cast<uint8_t>(x), static_cast<uint8_t>(y))) };
    if (into == INTO_BLOCKED) {
      if (!retry) return WAITING;
      retry = false;
      continue;
    }
    if (into == INTO_LOST) return update();
    break;
  }
  auto const state{ opeu_battle_state(game, element) };
  if ((Is(state, BattleState::BLITZ) && !opeu_battle_engaged(game, element)) || Is(state, BattleState::MOVE) || Is(state, BattleState::WITHDRAW)) {
    auto const& position{ Position(game, element) };
    if (position.goal_x == x && position.goal_y == y) return ARRIVED;
  }
  return STEPPED;
}

// Without ammunition the element reports it and stops; otherwise it turns toward its target (or its goal) and fights.
void opeu_battle_attack_or_hold(Game& game, uint16_t element, uint16_t target) {
  auto& data{ Data(game) };
  if (!Record<Unit>(data, opeu_battle_element_unit(game, element)).resources[0]) {
    opeu_battle_supply_notice(game, element, static_cast<uint8_t>(0));
    opeu_battle_update_element(game, element);
    return;
  }
  if (Turns(game, element)) {
    if (target) {
      FaceTarget(game, element, target);
    } else {
      auto const& position{ Position(game, element) };
      FaceCell(game, element, position.goal_x, position.goal_y);
    }
  }
  opeu_battle_loss_check(game, element, target);
}

// Carries on the element's current order.
void opeu_battle_continue_order(Game& game, uint16_t element) {
  auto order{ opeu_battle_order(game, element) };
  if (opeu_battle_engaged(game, element)) {
    auto const target{ Position(game, element).target };
    if (opeu_battle_target_in_range(game, element)) {
      if (ElementStrength(game, element) > order) {
        if (Turns(game, element)) FaceTarget(game, element, target);
        opeu_battle_set_order(game, element, ++order);
      } else {
        opeu_battle_attack_or_hold(game, element, target);
      }
    } else if (Is(opeu_battle_state(game, element), BattleState::ATTACK)) {
      opeu_battle_execute_order(game, element, order);
    }
  } else {
    auto const goal_x{ Position(game, element).goal_x };
    auto const goal_y{ Position(game, element).goal_y };
    if (opeu_battle_cell_in_range(game, element)) {
      if (ElementStrength(game, element) > order) {
        if (Turns(game, element)) FaceCell(game, element, goal_x, goal_y);
        opeu_battle_set_order(game, element, ++order);
      } else {
        opeu_battle_attack_or_hold(game, element, 0);
      }
    } else {
      opeu_battle_execute_order(game, element, order);
    }
  }
}

// The element advances on its order; on arrival its state is updated.
void opeu_battle_advance(Game& game, uint16_t element) {
  auto const order{ opeu_battle_order(game, element) };
  if (opeu_battle_execute_order(game, element, order) == ARRIVED) opeu_battle_update_element(game, element);
}

// frame: the routine's frame on the DS stack; the order runs below it.
void opeu_battle_seek_target(Game& game, uint16_t element) {
  auto order{ opeu_battle_order(game, element) };
  if (opeu_battle_engaged(game, element) && !opeu_battle_element_active(game, Position(game, element).target)) {
    opeu_battle_update_element(game, element);
    return;
  }
  if (auto const target{ opeu_battle_best_target(game, element) }) {
    if (ElementStrength(game, element) <= order) {
      opeu_battle_attack_or_hold(game, element, target);
    } else {
      if (Turns(game, element)) FaceTarget(game, element, target);
      opeu_battle_set_order(game, element, ++order);
    }
  } else if (opeu_battle_execute_order(game, element, order) == ARRIVED) {
    opeu_battle_update_element(game, element);
  }
}
