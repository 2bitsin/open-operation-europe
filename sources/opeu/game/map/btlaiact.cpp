// The computer's battle element actions: withdrawal to a map edge, ranged fire, the advance behind the unit's
// ranged elements, and the dispatch on the element's plan.
#include "map/btlaiact.hpp"
#include "common/battle-calls.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btlaient.hpp"
#include "map/btlaimov.hpp"
#include "map/btlaiunt.hpp"
#include "map/btlcalc.hpp"
#include "map/btllist.hpp"
#include "map/btlmove.hpp"
#include "map/btlpath.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <ranges>

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

[[nodiscard]] auto CanEnter(Game& game, uint8_t x, uint8_t y, uint16_t element) -> uint16_t {
  return opeu_battle_can_enter(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y), element);
}

// Floods the path costs from (from_x, from_y); nonzero when (x, y) is reached.
[[nodiscard]] auto Flood(Game& game, uint8_t x, uint8_t y, uint8_t from_x, uint8_t from_y, uint16_t element) -> uint16_t {
  return opeu_battle_path_flood(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y), static_cast<uint8_t>(from_x), static_cast<uint8_t>(from_y), element, static_cast<uint8_t>(1));
}

// The cost of the path flood's cheapest step from the cell (x, y).
[[nodiscard]] auto DownhillCost(Game const& game, uint8_t x, uint8_t y) -> uint16_t {
  return DownhillStep(game, { x, y }).cost;
}

// A signed 4-bit step: the low three bits with the fourth as the sign.
[[nodiscard]] auto Nibble(uint8_t value) -> int {
  return (value & 8) ? -(value & 7) : value;
}
} // namespace

uint16_t opeu_battle_ai_withdraw(Game& game, uint16_t element, uint16_t unit) {
  auto& data{ Data(game) };
  auto  side{ UnitDirection(game, unit) };
  if (side == BATTLE_NOT_ADJACENT) {
    if (data.battle_ai_entry_side[0] == NONE) return 0;
    side = data.battle_ai_entry_side[0];
  }
  if (side == NONE) return 0;
  uint16_t best{ NO_DISTANCE };
  uint8_t  best_x{ 0 };
  uint8_t  best_y{ 0 };
  for (auto const y : std::views::iota(uint8_t{ 0 }, BATTLE_WIDTH))
    for (auto const x : std::views::iota(uint8_t{ 0 }, BATTLE_WIDTH)) {
      if (x != 0 && x != BATTLE_EDGE && y != 0 && y != BATTLE_EDGE) continue;
      if ((BattleCellAt(data, x, y) >> std::to_underlying(BattleCell::EXIT_SHIFT)) - side != 1) continue;
      if (!CanEnter(game, x, y, element)) continue;
      if (opeu_battle_cell_occupied(game, x, y)) continue;
      auto const from_x{ PositionOf(data, element).x };
      auto const from_y{ PositionOf(data, element).y };
      if (!Flood(game, x, y, from_x, from_y, element)) continue;
      auto const distance{ DownhillCost(game, from_x, from_y) };
      if (distance < best) {
        best_x = x;
        best_y = y;
        best   = distance;
      }
    }
  if (best == NO_DISTANCE) return 0;
  opeu_battle_ai_move_to(game, GridCell{ best_x, best_y }, element);
  return 1;
}

uint16_t opeu_battle_ai_try_fire(Game& game, uint16_t element, uint16_t target) {
  auto& data{ Data(game) };
  if (!CanEnter(game, PositionOf(data, target).x, PositionOf(data, target).y, element)) return 0;
  if (!Flood(game, PositionOf(data, target).x, PositionOf(data, target).y, PositionOf(data, element).x, PositionOf(data, element).y, element)) return 0;
  PositionOf(data, element).state  = Bits(ElementTypeOf(data, element).category == Bits(Category::TANKS) ? BattleState::BLITZ : BattleState::ATTACK);
  PositionOf(data, element).target = target;
  opeu_battle_engage(game, element);
  opeu_battle_set_facing(game, element, NONE);
  return 1;
}

void opeu_battle_ai_fire_or_attack(Game& game, uint16_t element, uint16_t unit) {
  auto& data{ Data(game) };
  for (auto& other : Elements(data, Record<Unit>(data, unit))) {
    auto const near{ Near(data, other) };
    if (opeu_battle_element_active(game, near) && ElementTypeOf(data, near).gun_range > RANGED_MODE && opeu_battle_ai_try_fire(game, element, near)) return;
  }
  opeu_battle_ai_attack_near(game, element, unit);
}

auto opeu_battle_ai_try_move(Game& game, uint16_t element, GridCell cell) -> uint16_t {
  auto& data{ Data(game) };
  if (!CanEnter(game, cell.x, cell.y, element)) return 0;
  if (!Flood(game, cell.x, cell.y, PositionOf(data, element).x, PositionOf(data, element).y, element)) return 0;
  opeu_battle_ai_move_to(game, cell, element);
  return 1;
}

void opeu_battle_ai_advance(Game& game, uint16_t element) {
  auto&      data{ Data(game) };
  uint8_t    front_x{ NONE };
  uint8_t    front_y{ 0 };
  auto const unit{ opeu_battle_element_unit(game, element) };
  for (auto& other : Elements(data, Record<Unit>(data, unit))) {
    auto const near{ Near(data, other) };
    if (!opeu_battle_element_active(game, near)) continue;
    if (ElementTypeOf(data, near).gun_range <= RANGED_MODE) continue;
    front_x = PositionOf(data, near).x;
    front_y = PositionOf(data, near).y;
    break;
  }
  if (front_x == NONE) opeu_battle_ai_hold(game, element);
  auto direction{ UnitDirection(game, unit) };
  if (direction == BATTLE_NOT_ADJACENT) direction = static_cast<uint8_t>(7 - UnitDirection(game, opeu_battle_ai_unit_target(game, element)));
  uint16_t best{ NO_DISTANCE };
  uint8_t  best_x{ 0 };
  uint8_t  best_y{ 0 };
  for (auto const i : std::views::iota(uint8_t{ 0 }, OFFSETS_PER_DIRECTION)) {
    auto const offset{ Record<uint8_t>(data, static_cast<uint16_t>(Near(data, data.battle_ai_offsets) + direction * OFFSETS_PER_DIRECTION + i)) };
    auto const high{ static_cast<uint8_t>(offset >> 4) };
    auto const low{ static_cast<uint8_t>(offset & 0x0f) };
    auto const dx{ Nibble(high) + front_x };
    auto const dy{ Nibble(low) + front_y };
    if (dx < 0 || dx >= static_cast<int>(BATTLE_WIDTH) || dy < 0 || dy >= static_cast<int>(BATTLE_WIDTH)) continue;
    auto const x{ static_cast<uint8_t>(dx) };
    auto const y{ static_cast<uint8_t>(dy) };
    if (PositionOf(data, element).x == x && PositionOf(data, element).y == y) {
      opeu_battle_ai_hold(game, element);
      return;
    }
    if (opeu_battle_cell_occupied(game, x, y)) continue;
    if (!Flood(game, x, y, PositionOf(data, element).x, PositionOf(data, element).y, element)) continue;
    // The original measures from the offset's nibbles, not from the cell.
    auto const distance{ DownhillCost(game, high, low) };
    if (distance < best) {
      best   = distance;
      best_x = x;
      best_y = y;
    }
  }
  if (best != NO_DISTANCE && opeu_battle_ai_try_move(game, element, GridCell{ best_x, best_y })) return;
  opeu_battle_ai_hold(game, element);
}

void opeu_battle_ai_engage_best(Game& game, uint16_t element) {
  if (opeu_battle_best_target(game, element)) {
    opeu_battle_ai_hold(game, element);
    return;
  }
  if (auto const unit{ opeu_battle_ai_unit_target(game, element) }) {
    auto const cell{ opeu_battle_ai_random_cell(game, element, unit) };
    opeu_battle_ai_move_to(game, cell, element);
    return;
  }
  opeu_battle_ai_hold(game, element);
}

void opeu_battle_ai_act(Game& game, uint16_t element) {
  if (ElementTypeOf(Data(game), element).gun_range > RANGED_MODE)
    opeu_battle_ai_engage_best(game, element);
  else
    opeu_battle_ai_advance(game, element);
}

void opeu_battle_ai_think(Game& game, uint16_t element) {
  auto&      data{ Data(game) };
  auto const unit{ opeu_battle_ai_unit_target(game, element) };
  if (!unit) {
    opeu_battle_ai_hold(game, element);
    return;
  }
  auto const plan{ opeu_battle_ai_plan_low(game, element) };
  auto const profile{ opeu_battle_ai_profile(game, unit, 0) };
  auto const action{ Record<uint8_t>(data, static_cast<uint16_t>(Near(data, data.battle_ai_actions) + plan * 5u + profile)) };
  switch (static_cast<AiAction>(action)) {
  case AiAction::WITHDRAW:
    if (opeu_battle_ai_withdraw(game, element, unit)) break;
    opeu_battle_ai_attack_near(game, element, unit);
    break;
  case AiAction::ATTACK_NEAR:    opeu_battle_ai_attack_near(game, element, unit); break;
  case AiAction::ATTACK_MID:     opeu_battle_ai_attack_mid(game, element, unit); break;
  case AiAction::ATTACK_FAR:     opeu_battle_ai_attack_far(game, element, unit); break;
  case AiAction::FIRE_OR_ATTACK: opeu_battle_ai_fire_or_attack(game, element, unit); break;
  case AiAction::ACT:            opeu_battle_ai_act(game, element); break;
  default:                       break;
  }
}
