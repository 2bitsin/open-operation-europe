// The computer's battle element actions: withdrawal to a map edge, ranged fire, the advance behind the unit's
// ranged elements, and the dispatch on the element's plan.
//
// Callees that keep coordinates on the original stack (SS = DS) take sp, the modeled SP at their entry.
#include "types.hpp"
#include "map/btlaiact.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/memory.hpp"
#include "map/btlaient.hpp"
#include "map/btlaimov.hpp"
#include "map/btlaiunt.hpp"
#include "map/btllist.hpp"
#include "map/btlmove.hpp"
#include "map/btlunit.hpp"
#include "common/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// battle_ai_actions[plan * 5 + profile]: what an element does against its target unit.
enum class AiAction : uint8_t {
  ATTACK_MID     = 0,
  ATTACK_NEAR    = 1,
  ATTACK_FAR     = 2,
  FIRE_OR_ATTACK = 3,
  ACT            = 4,
  WITHDRAW       = 5,
};

constexpr uint16_t NO_DISTANCE{ 0xffff };
constexpr uint8_t  NONE{ 0xff };
constexpr uint8_t  BATTLE_EDGE{ BATTLE_WIDTH - 1 };
constexpr uint8_t  OFFSETS_PER_DIRECTION{ 5 };
// ElementType::mode above this fires from afar.
constexpr uint8_t RANGED_MODE{ 1 };
// Below the callers' sp: the frames of the callees.
constexpr uint16_t ATTACK_FRAME{ 12 };
constexpr uint16_t RANDOM_X_AT{ 6 };
constexpr uint16_t RANDOM_Y_AT{ 8 };
constexpr uint16_t ACT_FRAME{ 4 };
constexpr uint16_t THINK_FRAME{ 16 };

[[nodiscard]] auto TypeOf(DataSegment& data, uint16_t element) -> ElementType& {
  return data.element_types[Record<Element>(data, element).type];
}

[[nodiscard]] auto Direction(uint8_t* ds, BattleEvent event, uint16_t unit)
    -> uint8_t {
  return static_cast<uint8_t>(
      Call(ds, event, BattleRoutine::BATTLE_UNIT_DIRECTION, unit));
}

[[nodiscard]] auto CanEnter(uint8_t* ds, BattleEvent event, uint8_t x,
                            uint8_t y, uint16_t element) -> uint16_t {
  return Call(ds, event, BattleRoutine::BATTLE_CAN_ENTER, x, y, element);
}

// Floods the path costs from (from_x, from_y); nonzero when (x, y) is reached.
[[nodiscard]] auto Flood(uint8_t* ds, BattleEvent event, uint8_t x, uint8_t y,
                         uint8_t from_x, uint8_t from_y, uint16_t element)
    -> uint16_t {
  return Call(ds, event, BattleRoutine::BATTLE_PATH_FLOOD, x, y, from_x, from_y,
              element, 1);
}

[[nodiscard]] auto PathStep(uint8_t* ds, BattleEvent event, uint8_t x,
                            uint8_t y) -> uint16_t {
  return Call(ds, event, BattleRoutine::BATTLE_PATH_STEP, x, y);
}

// A signed 4-bit step: the low three bits with the fourth as the sign.
[[nodiscard]] auto Nibble(uint8_t value) -> int {
  return (value & 8) ? -(value & 7) : value;
}
} // namespace

uint16_t opeu_battle_ai_withdraw(uint8_t* ds, BattleEvent event,
                                 uint16_t element, uint16_t unit) {
  auto& data{ Data(ds) };
  auto  side{ Direction(ds, event, unit) };
  if (side == BATTLE_NOT_ADJACENT) {
    if (data.battle_ai_entry_side[0] == NONE)
      return 0;
    side = data.battle_ai_entry_side[0];
  }
  if (side == NONE)
    return 0;
  uint16_t best{ NO_DISTANCE };
  uint8_t  best_x{ 0 };
  uint8_t  best_y{ 0 };
  for (uint8_t y{ 0 }; y < BATTLE_WIDTH; ++y)
    for (uint8_t x{ 0 }; x < BATTLE_WIDTH; ++x) {
      if (x != 0 && x != BATTLE_EDGE && y != 0 && y != BATTLE_EDGE)
        continue;
      if ((BattleCellAt(data, x, y) >>
           std::to_underlying(BattleCell::EXIT_SHIFT)) -
              side !=
          1)
        continue;
      if (!CanEnter(ds, event, x, y, element))
        continue;
      if (opeu_battle_cell_occupied(ds, event, x, y))
        continue;
      auto const from_x{ PositionOf(data, element).x };
      auto const from_y{ PositionOf(data, element).y };
      if (!Flood(ds, event, x, y, from_x, from_y, element))
        continue;
      auto const distance{ PathStep(ds, event, from_x, from_y) };
      if (distance < best) {
        best_x = x;
        best_y = y;
        best   = distance;
      }
    }
  if (best == NO_DISTANCE)
    return 0;
  opeu_battle_ai_move_to(ds, event, best_x, best_y, element);
  return 1;
}

uint16_t opeu_battle_ai_try_fire(uint8_t* ds, BattleEvent event,
                                 uint16_t element, uint16_t target) {
  auto& data{ Data(ds) };
  if (!CanEnter(ds, event, PositionOf(data, target).x,
                PositionOf(data, target).y, element))
    return 0;
  if (!Flood(ds, event, PositionOf(data, target).x, PositionOf(data, target).y,
             PositionOf(data, element).x, PositionOf(data, element).y, element))
    return 0;
  PositionOf(data, element).state =
      Bits(TypeOf(data, element).category == Bits(Category::TANKS)
               ? BattleState::BLITZ
               : BattleState::ATTACK);
  PositionOf(data, element).target = target;
  opeu_battle_engage(ds, event, element);
  opeu_battle_set_facing(ds, event, element, NONE);
  return 1;
}

void opeu_battle_ai_fire_or_attack(uint8_t* ds, BattleEvent event,
                                   uint16_t element, uint16_t unit,
                                   uint16_t sp) {
  auto& data{ Data(ds) };
  for (auto& other : Elements(data, Record<Unit>(data, unit))) {
    auto const near{ Near(data, other) };
    if (opeu_battle_element_active(ds, event, near) &&
        TypeOf(data, near).mode > RANGED_MODE &&
        opeu_battle_ai_try_fire(ds, event, element, near))
      return;
  }
  opeu_battle_ai_attack_near(ds, event, element, unit, Local(sp, ATTACK_FRAME));
}

uint16_t opeu_battle_ai_try_move(uint8_t* ds, BattleEvent event,
                                 uint16_t element, uint8_t x, uint8_t y) {
  auto& data{ Data(ds) };
  if (!CanEnter(ds, event, x, y, element))
    return 0;
  if (!Flood(ds, event, x, y, PositionOf(data, element).x,
             PositionOf(data, element).y, element))
    return 0;
  opeu_battle_ai_move_to(ds, event, x, y, element);
  return 1;
}

void opeu_battle_ai_advance(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto&      data{ Data(ds) };
  uint8_t    front_x{ NONE };
  uint8_t    front_y{ 0 };
  auto const unit{ opeu_battle_element_unit(ds, event, element) };
  for (auto& other : Elements(data, Record<Unit>(data, unit))) {
    auto const near{ Near(data, other) };
    if (!opeu_battle_element_active(ds, event, near))
      continue;
    if (TypeOf(data, near).mode <= RANGED_MODE)
      continue;
    front_x = PositionOf(data, near).x;
    front_y = PositionOf(data, near).y;
    break;
  }
  if (front_x == NONE)
    opeu_battle_ai_hold(ds, event, element);
  auto direction{ Direction(ds, event, unit) };
  if (direction == BATTLE_NOT_ADJACENT)
    direction = static_cast<uint8_t>(
        7 -
        Direction(ds, event, opeu_battle_ai_unit_target(ds, event, element)));
  uint16_t best{ NO_DISTANCE };
  uint8_t  best_x{ 0 };
  uint8_t  best_y{ 0 };
  for (uint8_t i{ 0 }; i < OFFSETS_PER_DIRECTION; ++i) {
    auto const offset{ Record<uint8_t>(
        data, static_cast<uint16_t>(Near(data, data.battle_ai_offsets) +
                                    direction * OFFSETS_PER_DIRECTION + i)) };
    auto const high{ static_cast<uint8_t>(offset >> 4) };
    auto const low{ static_cast<uint8_t>(offset & 0x0f) };
    auto const dx{ Nibble(high) + front_x };
    auto const dy{ Nibble(low) + front_y };
    if (dx < 0 || dx >= static_cast<int>(BATTLE_WIDTH) || dy < 0 ||
        dy >= static_cast<int>(BATTLE_WIDTH))
      continue;
    auto const x{ static_cast<uint8_t>(dx) };
    auto const y{ static_cast<uint8_t>(dy) };
    if (PositionOf(data, element).x == x && PositionOf(data, element).y == y) {
      opeu_battle_ai_hold(ds, event, element);
      return;
    }
    if (opeu_battle_cell_occupied(ds, event, x, y))
      continue;
    if (!Flood(ds, event, x, y, PositionOf(data, element).x,
               PositionOf(data, element).y, element))
      continue;
    // The original measures from the offset's nibbles, not from the cell.
    auto const distance{ PathStep(ds, event, high, low) };
    if (distance < best) {
      best   = distance;
      best_x = x;
      best_y = y;
    }
  }
  if (best != NO_DISTANCE &&
      opeu_battle_ai_try_move(ds, event, element, best_x, best_y))
    return;
  opeu_battle_ai_hold(ds, event, element);
}

void opeu_battle_ai_engage_best(uint8_t* ds, BattleEvent event,
                                uint16_t element, uint16_t sp) {
  if (opeu_battle_best_target(ds, event, element)) {
    opeu_battle_ai_hold(ds, event, element);
    return;
  }
  if (auto const unit{ opeu_battle_ai_unit_target(ds, event, element) }) {
    auto const x_at{ Local(sp, RANDOM_X_AT) };
    auto const y_at{ Local(sp, RANDOM_Y_AT) };
    opeu_battle_ai_random_cell(ds, event, x_at, y_at, element, unit);
    opeu_battle_ai_move_to(ds, event, Byte(ds, x_at), Byte(ds, y_at), element);
    return;
  }
  opeu_battle_ai_hold(ds, event, element);
}

void opeu_battle_ai_act(uint8_t* ds, BattleEvent event, uint16_t element,
                        uint16_t sp) {
  if (TypeOf(Data(ds), element).mode > RANGED_MODE)
    opeu_battle_ai_engage_best(ds, event, element, Local(sp, ACT_FRAME));
  else
    opeu_battle_ai_advance(ds, event, element);
}

void opeu_battle_ai_think(uint8_t* ds, BattleEvent event, uint16_t element,
                          uint16_t sp) {
  auto&      data{ Data(ds) };
  auto const unit{ opeu_battle_ai_unit_target(ds, event, element) };
  if (!unit) {
    opeu_battle_ai_hold(ds, event, element);
    return;
  }
  auto const inner{ Local(sp, THINK_FRAME) };
  auto const plan{ opeu_battle_ai_plan_low(ds, event, element) };
  auto const profile{ opeu_battle_ai_profile(ds, event, unit, 0) };
  auto const action{ Record<uint8_t>(
      data, static_cast<uint16_t>(Near(data, data.battle_ai_actions) +
                                  plan * 5u + profile)) };
  switch (static_cast<AiAction>(action)) {
    case AiAction::WITHDRAW:
      if (opeu_battle_ai_withdraw(ds, event, element, unit))
        break;
      opeu_battle_ai_attack_near(ds, event, element, unit, inner);
      break;
    case AiAction::ATTACK_NEAR:
      opeu_battle_ai_attack_near(ds, event, element, unit, inner);
      break;
    case AiAction::ATTACK_MID:
      opeu_battle_ai_attack_mid(ds, event, element, unit, inner);
      break;
    case AiAction::ATTACK_FAR:
      opeu_battle_ai_attack_far(ds, event, element, unit, inner);
      break;
    case AiAction::FIRE_OR_ATTACK:
      opeu_battle_ai_fire_or_attack(ds, event, element, unit, inner);
      break;
    case AiAction::ACT: opeu_battle_ai_act(ds, event, element, inner); break;
    default:            break;
  }
}

// The services of the battle AI modules. The path step takes the addresses of two frame bytes it never reads back;
// the caller passes their values, which go to two modeled stack bytes.
uint16_t svc_battle_ai(uint8_t* ds, uint32_t address, const uint16_t* args,
                       unsigned count) {
  if (address == std::to_underlying(BattleRoutine::BATTLE_PATH_STEP)) {
    auto const cell{ port_scratch_enter(2) };
    Byte(ds, cell) = static_cast<uint8_t>(count > 0 ? args[0] : 0);
    Byte(ds, static_cast<uint16_t>(cell + 1u)) =
        static_cast<uint8_t>(count > 1 ? args[1] : 0);
    uint16_t const words[]{ static_cast<uint16_t>(cell + 1u), cell };
    auto const     result{ static_cast<uint16_t>(
        port_call(ds, address, words, 2)) };
    port_scratch_leave(2);
    return result;
  }
  return port_service(ds, address, args, count);
}

uint16_t svc_btlaiact(uint8_t* ds, uint32_t service, const uint16_t* args,
                      unsigned int count) {
  return svc_battle_ai(ds, service, args, count);
}
