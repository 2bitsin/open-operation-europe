// The computer's battle unit lookups: the plan and target unit of an element's list slot, the choice of a target
// element, and a random cell to make for.
//
// battle_ai_plans and battle_ai_targets run parallel to battle_lists (side * 9 + i). An element whose unit is in no
// list makes the plan and target lookups return its unit pointer (the low byte for the plans), as the original leaves
// it in AX.
#include "map/btlaiunt.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btlcalc.hpp"
#include "map/btllist.hpp"
#include "map/btlpath.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "map/btlaiact.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// opeu_battle_ai_pick_target's mode.
enum class PickMode : uint8_t {
  NEAREST        = 0, // the shortest path
  BEST_ODDS      = 1, // the best odds where the target's own odds are above 1
  SHORTEST_RANGE = 2, // the target type with the shortest range
};

constexpr uint16_t NO_DISTANCE{ 0xffff };
constexpr int      SLOT_COUNT{ 2 * BATTLE_LIST_LENGTH };

// The list slot (side * 9 + i) of the element's unit, or -1; unit gets the unit.
[[nodiscard]] auto FindSlot(Game& game, uint16_t element, uint16_t& unit) -> int {
  auto& data{ Data(game) };
  unit = opeu_battle_element_unit(game, element);
  for (int k{ 0 }; k < SLOT_COUNT; ++k)
    if (data.battle_lists[k] == unit) return k;
  return -1;
}

// Floods the path costs from the element toward the target; nonzero when the target is reached.
[[nodiscard]] auto Flood(Game& game, uint16_t element, uint16_t target) -> uint16_t {
  auto& data{ Data(game) };
  return opeu_battle_path_flood(game, static_cast<uint8_t>(PositionOf(data, target).x), static_cast<uint8_t>(PositionOf(data, target).y), static_cast<uint8_t>(PositionOf(data, element).x),
                                static_cast<uint8_t>(PositionOf(data, element).y), element, static_cast<uint8_t>(1));
}

// The range and rating opeu_battle_odds weighs for attacker against target.
struct Matchup {
  uint8_t range;
  uint8_t rating;
};
[[nodiscard]] auto MatchupOf(Game& game, uint16_t attacker, uint16_t target) -> Matchup {
  auto const range{ static_cast<uint8_t>(opeu_battle_element_range(game, attacker)) };
  auto const rating{ static_cast<uint8_t>(opeu_battle_element_rating(game, target, attacker)) };
  return { range, rating };
}
[[nodiscard]] auto Odds(Game& game, Matchup matchup) -> uint8_t {
  return static_cast<uint8_t>(opeu_battle_odds(game, static_cast<uint8_t>(matchup.rating), static_cast<uint8_t>(matchup.range)));
}
} // namespace

uint8_t opeu_battle_ai_plan_low(Game& game, uint16_t element) {
  uint16_t   unit;
  auto const k{ FindSlot(game, element, unit) };
  if (k < 0) return static_cast<uint8_t>(unit);
  return Data(game).battle_ai_plans[k] & 0x0f;
}

uint8_t opeu_battle_ai_plan_high(Game& game, uint16_t element) {
  uint16_t   unit;
  auto const k{ FindSlot(game, element, unit) };
  if (k < 0) return static_cast<uint8_t>(unit);
  return Data(game).battle_ai_plans[k] >> 4;
}

uint16_t opeu_battle_ai_unit_target(Game& game, uint16_t element) {
  uint16_t   unit;
  auto const k{ FindSlot(game, element, unit) };
  if (k < 0) return unit;
  auto const& data{ Data(game) };
  if (!opeu_battle_unit_active(game, data.battle_ai_targets[k])) return 0;
  return data.battle_ai_targets[k];
}

uint16_t opeu_battle_ai_pick_target(Game& game, uint16_t element, uint16_t list, uint8_t mode) {
  auto&    data{ Data(game) };
  uint16_t best{ NO_DISTANCE };
  uint16_t chosen{ 0 };
  uint16_t value{ 0 };
  for (auto& candidate : Elements(data, Record<Unit>(data, list))) {
    auto const target{ Near(data, candidate) };
    if (!opeu_battle_element_active(game, target) || !opeu_battle_element_flag2(game, target)) continue;
    switch (static_cast<PickMode>(mode)) {
    case PickMode::NEAREST:
      if (Flood(game, element, target)) {
        value = BattlePathStep(game, static_cast<uint8_t>(PositionOf(data, element).x), static_cast<uint8_t>(PositionOf(data, element).y));
        if (value < best) {
          best   = value;
          chosen = target;
        }
      }
      break;
    case PickMode::BEST_ODDS: {
      if (best == NO_DISTANCE) value = 0;
      uint16_t const odds{ Odds(game, MatchupOf(game, element, target)) };
      // The target's range and rating are taken in any case, its odds only when ours are better.
      auto const against{ MatchupOf(game, target, element) };
      if (value < odds && Odds(game, against) > 1 && Flood(game, element, target)) {
        value  = odds;
        chosen = target;
        best   = 0;
      }
      break;
    }
    case PickMode::SHORTEST_RANGE:
      value = data.element_types[candidate.type].defense;
      if (value < best && Flood(game, element, target)) {
        best   = value;
        chosen = target;
      }
      break;
    }
  }
  if (best == NO_DISTANCE) return 0;
  return chosen;
}

// A random cell the element can enter: on the unit's side of the field, or near the middle when the unit is not next
// to the battle.
auto opeu_battle_ai_random_cell(Game& game, uint16_t element, uint16_t unit) -> FieldCell {
  auto&      data{ Data(game) };
  auto const direction{ static_cast<uint8_t>(opeu_battle_unit_direction(game, unit)) };
  auto const pick{ [&](uint8_t column, uint8_t row, uint16_t percent) {
    return opeu_battle_can_enter(game, static_cast<uint8_t>(column), static_cast<uint8_t>(row), element) && port_rand_bounded(game, 100) < percent;
  } };
  auto const search{ [&](uint8_t& column, uint8_t& row) {
    if (direction != BATTLE_NOT_ADJACENT) {
      for (row = 0; row < BATTLE_WIDTH; ++row)
        for (column = 0; column < BATTLE_WIDTH; ++column)
          if (static_cast<uint8_t>((BattleCellAt(data, column, row) >> std::to_underlying(BattleCell::EXIT_SHIFT)) - 1) == direction && pick(column, row, 30)) return true;
    } else {
      for (row = 5; row < 9; ++row)
        for (column = 5; column < 9; ++column)
          if (pick(column, row, 15)) return true;
    }
    return false;
  } };
  uint8_t column;
  uint8_t row;
  while (!search(column, row)) {
  }
  return { column, row };
}
