// Battle order execution: advancing, attacking and holding.
#include "map/btlorder.hpp"
#include "common/battle-calls.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btllist.hpp"
#include "map/btlmove.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
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

void Face(Game& game, BattleEvent event, uint16_t element, uint8_t facing) {
  Call(game, event, BattleRoutine::BATTLE_ANIMATE_FACING, facing, element);
}

void FaceTarget(Game& game, BattleEvent event, uint16_t element, uint16_t target) {
  Face(game, event, element, static_cast<uint8_t>(Call(game, event, BattleRoutine::BATTLE_ELEMENT_FACING, target, element)));
}

void FaceCell(Game& game, BattleEvent event, uint16_t element, uint8_t x, uint8_t y) {
  auto const& position{ Position(game, element) };
  Face(game, event, element, static_cast<uint8_t>(Call(game, event, BattleRoutine::BATTLE_CELL_FACING, position.x, position.y, x, y)));
}
} // namespace

// Moves the element a step toward its goal or its target. order counts the element's waits this round. The cell it
// heads for is two DS words: the path step services receive their addresses.
uint8_t opeu_battle_execute_order(Game& game, BattleEvent event, uint16_t element, uint8_t order) {
  ServiceLocals const locals{ 4 };
  auto const          x_at{ locals.At(0) };
  auto const          y_at{ locals.At(2) };
  auto&               x{ Byte(game, x_at) };
  auto&               y{ Byte(game, y_at) };
  auto const          update{ [&] {
    opeu_battle_update_element(game, event, element);
    return UPDATED;
  } };
  auto const wait{ [&] {
    opeu_battle_set_order(game, event, element, ++order);
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
    auto const state{ opeu_battle_state(game, event, element) };
    if (Is(state, BattleState::MOVE) || Is(state, BattleState::WITHDRAW)) {
      head_for_goal();
    } else if (Is(state, BattleState::BLITZ)) {
      if (!opeu_battle_engaged(game, event, element)) {
        head_for_goal();
      } else {
        auto const target{ Position(game, element).target };
        if (!opeu_battle_element_active(game, event, target)) return update();
        head_for_target(target);
      }
    } else if (Is(state, BattleState::ATTACK)) {
      if (!opeu_battle_engaged(game, event, element))
        head_for_goal();
      else
        head_for_target(Position(game, element).target);
    }
    if (opeu_battle_facing(game, event, element) == NO_FACING) {
      switch (static_cast<uint8_t>(Call(game, event, BattleRoutine::BATTLE_NEXT_STEP, x_at, y_at, element))) {
      case STEP_WAIT: return wait();
      case STEP_SEARCH:
        if (!Call(game, event, BattleRoutine::BATTLE_ELEMENT_PATH_STEP, x_at, y_at, element)) return WAITING;
        break;
      case STEP_STUCK: return update();
      default:         break;
      }
      opeu_battle_set_strength(game, event, element, x, y);
      opeu_battle_set_facing(game, event, element, static_cast<uint8_t>(Call(game, event, BattleRoutine::BATTLE_MOVE_COST, x, y, element)));
    }
    if (opeu_battle_facing(game, event, element) > order) return wait();
    x = opeu_battle_strength_low(game, event, element);
    y = opeu_battle_strength_high(game, event, element);
    auto const into{ static_cast<uint8_t>(Call(game, event, BattleRoutine::BATTLE_STEP_INTO, x, y, element)) };
    if (into == INTO_BLOCKED) {
      if (!retry) return WAITING;
      retry = false;
      continue;
    }
    if (into == INTO_LOST) return update();
    break;
  }
  auto const state{ opeu_battle_state(game, event, element) };
  if ((Is(state, BattleState::BLITZ) && !opeu_battle_engaged(game, event, element)) || Is(state, BattleState::MOVE) || Is(state, BattleState::WITHDRAW)) {
    auto const& position{ Position(game, element) };
    if (position.goal_x == x && position.goal_y == y) return ARRIVED;
  }
  return STEPPED;
}

// Without ammunition the element reports it and stops; otherwise it turns toward its target (or its goal) and fights.
void opeu_battle_attack_or_hold(Game& game, BattleEvent event, uint16_t element, uint16_t target) {
  auto& data{ Data(game) };
  if (!Record<Unit>(data, opeu_battle_element_unit(game, event, element)).resources[0]) {
    Call(game, event, BattleRoutine::BATTLE_SUPPLY_NOTICE, 0, element);
    opeu_battle_update_element(game, event, element);
    return;
  }
  if (Turns(game, element)) {
    if (target) {
      FaceTarget(game, event, element, target);
    } else {
      auto const& position{ Position(game, element) };
      FaceCell(game, event, element, position.goal_x, position.goal_y);
    }
  }
  Call(game, event, BattleRoutine::BATTLE_LOSS_CHECK, target, element);
}

// Carries on the element's current order.
void opeu_battle_continue_order(Game& game, BattleEvent event, uint16_t element) {
  auto order{ opeu_battle_order(game, event, element) };
  if (opeu_battle_engaged(game, event, element)) {
    auto const target{ Position(game, element).target };
    if (Call(game, event, BattleRoutine::BATTLE_TARGET_IN_RANGE, element)) {
      if (ElementStrength(game, event, element) > order) {
        if (Turns(game, element)) FaceTarget(game, event, element, target);
        opeu_battle_set_order(game, event, element, ++order);
      } else {
        opeu_battle_attack_or_hold(game, event, element, target);
      }
    } else if (Is(opeu_battle_state(game, event, element), BattleState::ATTACK)) {
      opeu_battle_execute_order(game, event, element, order);
    }
  } else {
    auto const goal_x{ Position(game, element).goal_x };
    auto const goal_y{ Position(game, element).goal_y };
    if (Call(game, event, BattleRoutine::BATTLE_CELL_IN_RANGE, element)) {
      if (ElementStrength(game, event, element) > order) {
        if (Turns(game, element)) FaceCell(game, event, element, goal_x, goal_y);
        opeu_battle_set_order(game, event, element, ++order);
      } else {
        opeu_battle_attack_or_hold(game, event, element, 0);
      }
    } else {
      opeu_battle_execute_order(game, event, element, order);
    }
  }
}

// The element advances on its order; on arrival its state is updated.
void opeu_battle_advance(Game& game, BattleEvent event, uint16_t element) {
  auto const order{ opeu_battle_order(game, event, element) };
  if (opeu_battle_execute_order(game, event, element, order) == ARRIVED) opeu_battle_update_element(game, event, element);
}

// frame: the routine's frame on the DS stack; the order runs below it.
void opeu_battle_seek_target(Game& game, BattleEvent event, uint16_t element) {
  auto order{ opeu_battle_order(game, event, element) };
  if (opeu_battle_engaged(game, event, element) && !opeu_battle_element_active(game, event, Position(game, element).target)) {
    opeu_battle_update_element(game, event, element);
    return;
  }
  if (auto const target{ opeu_battle_best_target(game, event, element) }) {
    if (ElementStrength(game, event, element) <= order) {
      opeu_battle_attack_or_hold(game, event, element, target);
    } else {
      if (Turns(game, element)) FaceTarget(game, event, element, target);
      opeu_battle_set_order(game, event, element, ++order);
    }
  } else if (opeu_battle_execute_order(game, event, element, order) == ARRIVED) {
    opeu_battle_update_element(game, event, element);
  }
}

uint16_t svc_btlorder(Game& game, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(game, address, args, count);
}

auto port_routine_opeu_battle_advance(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_advance(game, svc_btlorder, Arg(args, count, 0));
  return 0;
}

auto port_routine_opeu_battle_attack_or_hold(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_attack_or_hold(game, svc_btlorder, Arg(args, count, 1), Arg(args, count, 0));
  return 0;
}

auto port_routine_opeu_battle_continue_order(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_continue_order(game, svc_btlorder, Arg(args, count, 0));
  return 0;
}

auto port_routine_opeu_battle_execute_order(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_battle_execute_order(game, svc_btlorder, Arg(args, count, 1), static_cast<uint8_t>(Arg(args, count, 0)));
}

auto port_routine_opeu_battle_seek_target(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_seek_target(game, svc_btlorder, Arg(args, count, 0));
  return 0;
}
