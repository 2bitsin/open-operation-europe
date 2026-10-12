// Battle order execution: advancing, attacking and holding.
#include "types.hpp"
#include "map/btlorder.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "map/btllist.hpp"
#include "map/btlmove.hpp"
#include "map/btlunit.hpp"
#include "types/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// Below opeu_battle_execute_order's frame: the cell it heads for, whose bytes the path steps receive by address.
constexpr uint16_t CELL_X_AT{ 6 };
constexpr uint16_t CELL_Y_AT{ 8 };
// The frames below its callers' that opeu_battle_execute_order runs on.
constexpr uint16_t CONTINUE_FRAME{ 0x12 };
constexpr uint16_t ADVANCE_FRAME{ 0x0c };
constexpr uint16_t SEEK_FRAME{ 0x0e };
constexpr uint8_t  NO_FACING{ 0xff };

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

[[nodiscard]] auto Position(uint8_t* ds, uint16_t element) -> BattlePosition& {
  return PositionOf(Data(ds), element);
}

// Vehicles turn to face where they fire or go; infantry does not.
[[nodiscard]] auto Turns(uint8_t* ds, uint16_t element) -> bool {
  auto const& data{ Data(ds) };
  return data.element_types[Record<Element>(data, element).type].category != 0;
}

[[nodiscard]] auto Strength(uint8_t* ds, BattleEvent event, uint16_t element)
    -> uint8_t {
  return static_cast<uint8_t>(
      Call(ds, event, BattleRoutine::BATTLE_ELEMENT_STRENGTH, element));
}

void Face(uint8_t* ds, BattleEvent event, uint16_t element, uint8_t facing) {
  Call(ds, event, BattleRoutine::BATTLE_ANIMATE_FACING, facing, element);
}

void FaceTarget(uint8_t* ds, BattleEvent event, uint16_t element,
                uint16_t target) {
  Face(ds, event, element,
       static_cast<uint8_t>(Call(
           ds, event, BattleRoutine::BATTLE_ELEMENT_FACING, target, element)));
}

void FaceCell(uint8_t* ds, BattleEvent event, uint16_t element, uint8_t x,
              uint8_t y) {
  auto const& position{ Position(ds, element) };
  Face(ds, event, element,
       static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_CELL_FACING,
                                 position.x, position.y, x, y)));
}
} // namespace

// Moves the element a step toward its goal or its target. order counts the element's waits this round. frame: the
// routine's frame on the DS stack; the cell it heads for lives there.
uint8_t opeu_battle_execute_order(uint8_t* ds, BattleEvent event,
                                  uint16_t element, uint8_t order,
                                  uint16_t frame) {
  auto const x_at{ Local(frame, CELL_X_AT) };
  auto const y_at{ Local(frame, CELL_Y_AT) };
  auto&      x{ Byte(ds, x_at) };
  auto&      y{ Byte(ds, y_at) };
  auto const update{ [&] {
    opeu_battle_update_element(ds, event, element);
    return UPDATED;
  } };
  auto const wait{ [&] {
    opeu_battle_set_order(ds, event, element, ++order);
    return WAITING;
  } };
  auto const head_for_goal{ [&] {
    auto const& position{ Position(ds, element) };
    x = position.goal_x;
    y = position.goal_y;
  } };
  auto const head_for_target{ [&](uint16_t target) {
    auto const& position{ Position(ds, target) };
    x = position.x;
    y = position.y;
  } };
  bool retry{ true };
  for (;;) {
    auto const state{ opeu_battle_state(ds, event, element) };
    if (Is(state, BattleState::MOVE) || Is(state, BattleState::WITHDRAW)) {
      head_for_goal();
    } else if (Is(state, BattleState::BLITZ)) {
      if (!opeu_battle_engaged(ds, event, element)) {
        head_for_goal();
      } else {
        auto const target{ Position(ds, element).target };
        if (!opeu_battle_element_active(ds, event, target))
          return update();
        head_for_target(target);
      }
    } else if (Is(state, BattleState::ATTACK)) {
      if (!opeu_battle_engaged(ds, event, element))
        head_for_goal();
      else
        head_for_target(Position(ds, element).target);
    }
    if (opeu_battle_facing(ds, event, element) == NO_FACING) {
      switch (static_cast<uint8_t>(Call(
          ds, event, BattleRoutine::BATTLE_NEXT_STEP, x_at, y_at, element))) {
        case STEP_WAIT: return wait();
        case STEP_SEARCH:
          if (!Call(ds, event, BattleRoutine::BATTLE_ELEMENT_PATH_STEP, x_at,
                    y_at, element))
            return WAITING;
          break;
        case STEP_STUCK: return update();
        default:         break;
      }
      opeu_battle_set_strength(ds, event, element, x, y);
      opeu_battle_set_facing(
          ds, event, element,
          static_cast<uint8_t>(
              Call(ds, event, BattleRoutine::BATTLE_MOVE_COST, x, y, element)));
    }
    if (opeu_battle_facing(ds, event, element) > order)
      return wait();
    x = opeu_battle_strength_low(ds, event, element);
    y = opeu_battle_strength_high(ds, event, element);
    auto const into{ static_cast<uint8_t>(
        Call(ds, event, BattleRoutine::BATTLE_STEP_INTO, x, y, element)) };
    if (into == INTO_BLOCKED) {
      if (!retry)
        return WAITING;
      retry = false;
      continue;
    }
    if (into == INTO_LOST)
      return update();
    break;
  }
  auto const state{ opeu_battle_state(ds, event, element) };
  if ((Is(state, BattleState::BLITZ) &&
       !opeu_battle_engaged(ds, event, element)) ||
      Is(state, BattleState::MOVE) || Is(state, BattleState::WITHDRAW)) {
    auto const& position{ Position(ds, element) };
    if (position.goal_x == x && position.goal_y == y)
      return ARRIVED;
  }
  return STEPPED;
}

// Without ammunition the element reports it and stops; otherwise it turns toward its target (or its goal) and fights.
void opeu_battle_attack_or_hold(uint8_t* ds, BattleEvent event,
                                uint16_t element, uint16_t target) {
  auto& data{ Data(ds) };
  if (!Record<Unit>(data, opeu_battle_element_unit(ds, event, element))
           .resources[0]) {
    Call(ds, event, BattleRoutine::BATTLE_SUPPLY_NOTICE, 0, element);
    opeu_battle_update_element(ds, event, element);
    return;
  }
  if (Turns(ds, element)) {
    if (target) {
      FaceTarget(ds, event, element, target);
    } else {
      auto const& position{ Position(ds, element) };
      FaceCell(ds, event, element, position.goal_x, position.goal_y);
    }
  }
  Call(ds, event, BattleRoutine::BATTLE_LOSS_CHECK, target, element);
}

// frame: the routine's frame on the DS stack; the order runs below it.
void opeu_battle_continue_order(uint8_t* ds, BattleEvent event,
                                uint16_t element, uint16_t frame) {
  auto order{ opeu_battle_order(ds, event, element) };
  if (opeu_battle_engaged(ds, event, element)) {
    auto const target{ Position(ds, element).target };
    if (Call(ds, event, BattleRoutine::BATTLE_TARGET_IN_RANGE, element)) {
      if (Strength(ds, event, element) > order) {
        if (Turns(ds, element))
          FaceTarget(ds, event, element, target);
        opeu_battle_set_order(ds, event, element, ++order);
      } else {
        opeu_battle_attack_or_hold(ds, event, element, target);
      }
    } else if (Is(opeu_battle_state(ds, event, element), BattleState::ATTACK)) {
      opeu_battle_execute_order(ds, event, element, order,
                                Local(frame, CONTINUE_FRAME));
    }
  } else {
    auto const goal_x{ Position(ds, element).goal_x };
    auto const goal_y{ Position(ds, element).goal_y };
    if (Call(ds, event, BattleRoutine::BATTLE_CELL_IN_RANGE, element)) {
      if (Strength(ds, event, element) > order) {
        if (Turns(ds, element))
          FaceCell(ds, event, element, goal_x, goal_y);
        opeu_battle_set_order(ds, event, element, ++order);
      } else {
        opeu_battle_attack_or_hold(ds, event, element, 0);
      }
    } else {
      opeu_battle_execute_order(ds, event, element, order,
                                Local(frame, CONTINUE_FRAME));
    }
  }
}

// frame: the routine's frame on the DS stack; the order runs below it.
void opeu_battle_advance(uint8_t* ds, BattleEvent event, uint16_t element,
                         uint16_t frame) {
  auto const order{ opeu_battle_order(ds, event, element) };
  if (opeu_battle_execute_order(ds, event, element, order,
                                Local(frame, ADVANCE_FRAME)) == ARRIVED)
    opeu_battle_update_element(ds, event, element);
}

// frame: the routine's frame on the DS stack; the order runs below it.
void opeu_battle_seek_target(uint8_t* ds, BattleEvent event, uint16_t element,
                             uint16_t frame) {
  auto order{ opeu_battle_order(ds, event, element) };
  if (opeu_battle_engaged(ds, event, element) &&
      !opeu_battle_element_active(ds, event, Position(ds, element).target)) {
    opeu_battle_update_element(ds, event, element);
    return;
  }
  if (auto const target{ opeu_battle_best_target(ds, event, element) }) {
    if (Strength(ds, event, element) <= order) {
      opeu_battle_attack_or_hold(ds, event, element, target);
    } else {
      if (Turns(ds, element))
        FaceTarget(ds, event, element, target);
      opeu_battle_set_order(ds, event, element, ++order);
    }
  } else if (opeu_battle_execute_order(ds, event, element, order,
                                       Local(frame, SEEK_FRAME)) == ARRIVED) {
    opeu_battle_update_element(ds, event, element);
  }
}

uint16_t svc_btlorder(uint8_t* ds, uint32_t address, const uint16_t* args,
                      unsigned int count) {
  return port_service(ds, address, args, count);
}

auto port_routine_opeu_battle_advance(std::uint8_t*        ds,
                                      std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  WithBattleStack([&](uint16_t sp) {
    opeu_battle_advance(ds, svc_btlorder, Arg(args, count, 0), BattleFrame(sp));
  });
  return 0;
}

auto port_routine_opeu_battle_attack_or_hold(std::uint8_t*        ds,
                                             std::uint16_t const* args,
                                             unsigned count) -> std::uint32_t {
  opeu_battle_attack_or_hold(ds, svc_btlorder, Arg(args, count, 1),
                             Arg(args, count, 0));
  return 0;
}

auto port_routine_opeu_battle_continue_order(std::uint8_t*        ds,
                                             std::uint16_t const* args,
                                             unsigned count) -> std::uint32_t {
  WithBattleStack([&](uint16_t sp) {
    opeu_battle_continue_order(ds, svc_btlorder, Arg(args, count, 0),
                               BattleFrame(sp));
  });
  return 0;
}

auto port_routine_opeu_battle_execute_order(std::uint8_t*        ds,
                                            std::uint16_t const* args,
                                            unsigned count) -> std::uint32_t {
  return WithBattleStack([&](uint16_t sp) {
    return opeu_battle_execute_order(ds, svc_btlorder, Arg(args, count, 1),
                                     static_cast<uint8_t>(Arg(args, count, 0)),
                                     BattleFrame(sp));
  });
}

auto port_routine_opeu_battle_seek_target(std::uint8_t*        ds,
                                          std::uint16_t const* args,
                                          unsigned count) -> std::uint32_t {
  WithBattleStack([&](uint16_t sp) {
    opeu_battle_seek_target(ds, svc_btlorder, Arg(args, count, 0),
                            BattleFrame(sp));
  });
  return 0;
}
