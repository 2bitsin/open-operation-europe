// The computer's battle element actions: withdrawal to a map edge, ranged fire, the advance behind the unit's
// ranged elements, and the dispatch on the element's plan.
#include "map/btlaiact.hpp"
#include "common/battle-calls.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btlaient.hpp"
#include "map/btlaimov.hpp"
#include "map/btlaiunt.hpp"
#include "map/btllist.hpp"
#include "map/btlmove.hpp"
#include "map/btlpath.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "port.hpp"
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

[[nodiscard]] auto CanEnter(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint16_t element) -> uint16_t {
  return Call(game, event, BattleRoutine::BATTLE_CAN_ENTER, x, y, element);
}

// Floods the path costs from (from_x, from_y); nonzero when (x, y) is reached.
[[nodiscard]] auto Flood(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint8_t from_x, uint8_t from_y, uint16_t element) -> uint16_t {
  return Call(game, event, BattleRoutine::BATTLE_PATH_FLOOD, x, y, from_x, from_y, element, 1);
}

[[nodiscard]] auto PathStep(Game& game, BattleEvent event, uint8_t x, uint8_t y) -> uint16_t {
  return Call(game, event, BattleRoutine::BATTLE_PATH_STEP, x, y);
}

// A signed 4-bit step: the low three bits with the fourth as the sign.
[[nodiscard]] auto Nibble(uint8_t value) -> int {
  return (value & 8) ? -(value & 7) : value;
}
} // namespace

uint16_t opeu_battle_ai_withdraw(Game& game, BattleEvent event, uint16_t element, uint16_t unit) {
  auto& data{ Data(game) };
  auto  side{ UnitDirection(game, event, unit) };
  if (side == BATTLE_NOT_ADJACENT) {
    if (data.battle_ai_entry_side[0] == NONE) return 0;
    side = data.battle_ai_entry_side[0];
  }
  if (side == NONE) return 0;
  uint16_t best{ NO_DISTANCE };
  uint8_t  best_x{ 0 };
  uint8_t  best_y{ 0 };
  for (uint8_t y{ 0 }; y < BATTLE_WIDTH; ++y)
    for (uint8_t x{ 0 }; x < BATTLE_WIDTH; ++x) {
      if (x != 0 && x != BATTLE_EDGE && y != 0 && y != BATTLE_EDGE) continue;
      if ((BattleCellAt(data, x, y) >> std::to_underlying(BattleCell::EXIT_SHIFT)) - side != 1) continue;
      if (!CanEnter(game, event, x, y, element)) continue;
      if (opeu_battle_cell_occupied(game, event, x, y)) continue;
      auto const from_x{ PositionOf(data, element).x };
      auto const from_y{ PositionOf(data, element).y };
      if (!Flood(game, event, x, y, from_x, from_y, element)) continue;
      auto const distance{ PathStep(game, event, from_x, from_y) };
      if (distance < best) {
        best_x = x;
        best_y = y;
        best   = distance;
      }
    }
  if (best == NO_DISTANCE) return 0;
  opeu_battle_ai_move_to(game, event, best_x, best_y, element);
  return 1;
}

uint16_t opeu_battle_ai_try_fire(Game& game, BattleEvent event, uint16_t element, uint16_t target) {
  auto& data{ Data(game) };
  if (!CanEnter(game, event, PositionOf(data, target).x, PositionOf(data, target).y, element)) return 0;
  if (!Flood(game, event, PositionOf(data, target).x, PositionOf(data, target).y, PositionOf(data, element).x, PositionOf(data, element).y, element)) return 0;
  PositionOf(data, element).state  = Bits(ElementTypeOf(data, element).category == Bits(Category::TANKS) ? BattleState::BLITZ : BattleState::ATTACK);
  PositionOf(data, element).target = target;
  opeu_battle_engage(game, event, element);
  opeu_battle_set_facing(game, event, element, NONE);
  return 1;
}

void opeu_battle_ai_fire_or_attack(Game& game, BattleEvent event, uint16_t element, uint16_t unit) {
  auto& data{ Data(game) };
  for (auto& other : Elements(data, Record<Unit>(data, unit))) {
    auto const near{ Near(data, other) };
    if (opeu_battle_element_active(game, event, near) && ElementTypeOf(data, near).gun_range > RANGED_MODE && opeu_battle_ai_try_fire(game, event, element, near)) return;
  }
  opeu_battle_ai_attack_near(game, event, element, unit);
}

uint16_t opeu_battle_ai_try_move(Game& game, BattleEvent event, uint16_t element, uint8_t x, uint8_t y) {
  auto& data{ Data(game) };
  if (!CanEnter(game, event, x, y, element)) return 0;
  if (!Flood(game, event, x, y, PositionOf(data, element).x, PositionOf(data, element).y, element)) return 0;
  opeu_battle_ai_move_to(game, event, x, y, element);
  return 1;
}

void opeu_battle_ai_advance(Game& game, BattleEvent event, uint16_t element) {
  auto&      data{ Data(game) };
  uint8_t    front_x{ NONE };
  uint8_t    front_y{ 0 };
  auto const unit{ opeu_battle_element_unit(game, event, element) };
  for (auto& other : Elements(data, Record<Unit>(data, unit))) {
    auto const near{ Near(data, other) };
    if (!opeu_battle_element_active(game, event, near)) continue;
    if (ElementTypeOf(data, near).gun_range <= RANGED_MODE) continue;
    front_x = PositionOf(data, near).x;
    front_y = PositionOf(data, near).y;
    break;
  }
  if (front_x == NONE) opeu_battle_ai_hold(game, event, element);
  auto direction{ UnitDirection(game, event, unit) };
  if (direction == BATTLE_NOT_ADJACENT) direction = static_cast<uint8_t>(7 - UnitDirection(game, event, opeu_battle_ai_unit_target(game, event, element)));
  uint16_t best{ NO_DISTANCE };
  uint8_t  best_x{ 0 };
  uint8_t  best_y{ 0 };
  for (uint8_t i{ 0 }; i < OFFSETS_PER_DIRECTION; ++i) {
    auto const offset{ Record<uint8_t>(data, static_cast<uint16_t>(Near(data, data.battle_ai_offsets) + direction * OFFSETS_PER_DIRECTION + i)) };
    auto const high{ static_cast<uint8_t>(offset >> 4) };
    auto const low{ static_cast<uint8_t>(offset & 0x0f) };
    auto const dx{ Nibble(high) + front_x };
    auto const dy{ Nibble(low) + front_y };
    if (dx < 0 || dx >= static_cast<int>(BATTLE_WIDTH) || dy < 0 || dy >= static_cast<int>(BATTLE_WIDTH)) continue;
    auto const x{ static_cast<uint8_t>(dx) };
    auto const y{ static_cast<uint8_t>(dy) };
    if (PositionOf(data, element).x == x && PositionOf(data, element).y == y) {
      opeu_battle_ai_hold(game, event, element);
      return;
    }
    if (opeu_battle_cell_occupied(game, event, x, y)) continue;
    if (!Flood(game, event, x, y, PositionOf(data, element).x, PositionOf(data, element).y, element)) continue;
    // The original measures from the offset's nibbles, not from the cell.
    auto const distance{ PathStep(game, event, high, low) };
    if (distance < best) {
      best   = distance;
      best_x = x;
      best_y = y;
    }
  }
  if (best != NO_DISTANCE && opeu_battle_ai_try_move(game, event, element, best_x, best_y)) return;
  opeu_battle_ai_hold(game, event, element);
}

void opeu_battle_ai_engage_best(Game& game, BattleEvent event, uint16_t element) {
  if (opeu_battle_best_target(game, event, element)) {
    opeu_battle_ai_hold(game, event, element);
    return;
  }
  if (auto const unit{ opeu_battle_ai_unit_target(game, event, element) }) {
    auto const cell{ opeu_battle_ai_random_cell(game, event, element, unit) };
    opeu_battle_ai_move_to(game, event, cell.x, cell.y, element);
    return;
  }
  opeu_battle_ai_hold(game, event, element);
}

void opeu_battle_ai_act(Game& game, BattleEvent event, uint16_t element) {
  if (ElementTypeOf(Data(game), element).gun_range > RANGED_MODE)
    opeu_battle_ai_engage_best(game, event, element);
  else
    opeu_battle_ai_advance(game, event, element);
}

void opeu_battle_ai_think(Game& game, BattleEvent event, uint16_t element) {
  auto&      data{ Data(game) };
  auto const unit{ opeu_battle_ai_unit_target(game, event, element) };
  if (!unit) {
    opeu_battle_ai_hold(game, event, element);
    return;
  }
  auto const plan{ opeu_battle_ai_plan_low(game, event, element) };
  auto const profile{ opeu_battle_ai_profile(game, event, unit, 0) };
  auto const action{ Record<uint8_t>(data, static_cast<uint16_t>(Near(data, data.battle_ai_actions) + plan * 5u + profile)) };
  switch (static_cast<AiAction>(action)) {
  case AiAction::WITHDRAW:
    if (opeu_battle_ai_withdraw(game, event, element, unit)) break;
    opeu_battle_ai_attack_near(game, event, element, unit);
    break;
  case AiAction::ATTACK_NEAR:    opeu_battle_ai_attack_near(game, event, element, unit); break;
  case AiAction::ATTACK_MID:     opeu_battle_ai_attack_mid(game, event, element, unit); break;
  case AiAction::ATTACK_FAR:     opeu_battle_ai_attack_far(game, event, element, unit); break;
  case AiAction::FIRE_OR_ATTACK: opeu_battle_ai_fire_or_attack(game, event, element, unit); break;
  case AiAction::ACT:            opeu_battle_ai_act(game, event, element); break;
  default:                       break;
  }
}

namespace opeu::game::detail::btlaiact {
auto BattlePathStep(Game& game, std::uint8_t x, std::uint8_t y) -> std::uint16_t {
  ServiceLocals const locals{ 2 };
  auto const          x_at{ locals.At(0) };
  auto const          y_at{ static_cast<std::uint16_t>(x_at + 1u) };
  Byte(game, x_at) = x;
  Byte(game, y_at) = y;
  return opeu_battle_path_step(game, svc_btlpath, x_at, y_at);
}
} // namespace opeu::game::detail::btlaiact

// The services of the battle AI modules. The path step takes the addresses of two frame bytes it never reads back;
// the caller passes their values.
uint16_t svc_battle_ai(Game& game, uint32_t address, const uint16_t* args, unsigned count) {
  if (address == std::to_underlying(BattleRoutine::BATTLE_PATH_STEP)) return BattlePathStep(game, static_cast<uint8_t>(Arg(args, count, 0)), static_cast<uint8_t>(Arg(args, count, 1)));
  return port_service(game, address, args, count);
}

uint16_t svc_btlaiact(Game& game, uint32_t service, const uint16_t* args, unsigned int count) {
  return svc_battle_ai(game, service, args, count);
}
