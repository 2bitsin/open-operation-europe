// The computer's battle unit lookups: the plan and target unit of an element's list slot, the choice of a target
// element, and a random cell to make for.
//
// battle_ai_plans and battle_ai_targets run parallel to battle_lists (side * 9 + i). An element whose unit is in no
// list makes the plan and target lookups return its unit pointer (the low byte for the plans), as the original leaves
// it in AX.
#include "map/btlaiunt.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btllist.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "glue.hpp"
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
[[nodiscard]] auto FindSlot(uint8_t* ds, BattleEvent event, uint16_t element, uint16_t& unit) -> int {
  auto& data{ Data(ds) };
  unit = opeu_battle_element_unit(ds, event, element);
  for (int k{ 0 }; k < SLOT_COUNT; ++k)
    if (data.battle_lists[k] == unit) return k;
  return -1;
}

// Floods the path costs from the element toward the target; nonzero when the target is reached.
[[nodiscard]] auto Flood(uint8_t* ds, BattleEvent event, uint16_t element, uint16_t target) -> uint16_t {
  auto& data{ Data(ds) };
  return Call(ds, event, BattleRoutine::BATTLE_PATH_FLOOD, PositionOf(data, target).x, PositionOf(data, target).y,
              PositionOf(data, element).x, PositionOf(data, element).y, element, 1);
}

// The range and rating opeu_battle_odds weighs for attacker against target.
struct Matchup {
  uint8_t range;
  uint8_t rating;
};
[[nodiscard]] auto MatchupOf(uint8_t* ds, BattleEvent event, uint16_t attacker, uint16_t target) -> Matchup {
  auto const range{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_ELEMENT_RANGE, attacker)) };
  auto const rating{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_ELEMENT_RATING, attacker, target)) };
  return { range, rating };
}
[[nodiscard]] auto Odds(uint8_t* ds, BattleEvent event, Matchup matchup) -> uint8_t {
  return static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_ODDS, matchup.rating, matchup.range));
}
} // namespace

uint8_t opeu_battle_ai_plan_low(uint8_t* ds, BattleEvent event, uint16_t element) {
  uint16_t   unit;
  auto const k{ FindSlot(ds, event, element, unit) };
  if (k < 0) return static_cast<uint8_t>(unit);
  return Data(ds).battle_ai_plans[k] & 0x0f;
}

uint8_t opeu_battle_ai_plan_high(uint8_t* ds, BattleEvent event, uint16_t element) {
  uint16_t   unit;
  auto const k{ FindSlot(ds, event, element, unit) };
  if (k < 0) return static_cast<uint8_t>(unit);
  return Data(ds).battle_ai_plans[k] >> 4;
}

uint16_t opeu_battle_ai_unit_target(uint8_t* ds, BattleEvent event, uint16_t element) {
  uint16_t   unit;
  auto const k{ FindSlot(ds, event, element, unit) };
  if (k < 0) return unit;
  auto const& data{ Data(ds) };
  if (!opeu_battle_unit_active(ds, event, data.battle_ai_targets[k])) return 0;
  return data.battle_ai_targets[k];
}

uint16_t opeu_battle_ai_pick_target(uint8_t* ds, BattleEvent event, uint16_t element, uint16_t list, uint8_t mode) {
  auto&    data{ Data(ds) };
  uint16_t best{ NO_DISTANCE };
  uint16_t chosen{ 0 };
  uint16_t value{ 0 };
  for (auto& candidate : Elements(data, Record<Unit>(data, list))) {
    auto const target{ Near(data, candidate) };
    if (!opeu_battle_element_active(ds, event, target) || !opeu_battle_element_flag2(ds, event, target)) continue;
    switch (static_cast<PickMode>(mode)) {
    case PickMode::NEAREST:
      if (Flood(ds, event, element, target)) {
        value = Call(ds, event, BattleRoutine::BATTLE_PATH_STEP, PositionOf(data, element).x,
                     PositionOf(data, element).y);
        if (value < best) {
          best   = value;
          chosen = target;
        }
      }
      break;
    case PickMode::BEST_ODDS: {
      if (best == NO_DISTANCE) value = 0;
      uint16_t const odds{ Odds(ds, event, MatchupOf(ds, event, element, target)) };
      // The target's range and rating are taken in any case, its odds only when ours are better.
      auto const against{ MatchupOf(ds, event, target, element) };
      if (value < odds && Odds(ds, event, against) > 1 && Flood(ds, event, element, target)) {
        value  = odds;
        chosen = target;
        best   = 0;
      }
      break;
    }
    case PickMode::SHORTEST_RANGE:
      value = data.element_types[candidate.type].range;
      if (value < best && Flood(ds, event, element, target)) {
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
// to the battle; x and y are the DS offsets of the result bytes.
void opeu_battle_ai_random_cell(uint8_t* ds, BattleEvent event, uint16_t x, uint16_t y, uint16_t element,
                                uint16_t unit) {
  auto&      data{ Data(ds) };
  auto const direction{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_UNIT_DIRECTION, unit)) };
  auto const pick{ [&](uint8_t column, uint8_t row, uint16_t percent) {
    return Call(ds, event, BattleRoutine::BATTLE_CAN_ENTER, column, row, element)
           && Call(ds, event, BattleRoutine::RAND_BOUNDED, 100) < percent;
  } };
  auto const search{ [&](uint8_t& column, uint8_t& row) {
    if (direction != BATTLE_NOT_ADJACENT) {
      for (row = 0; row < BATTLE_WIDTH; ++row)
        for (column = 0; column < BATTLE_WIDTH; ++column)
          if (static_cast<uint8_t>((BattleCellAt(data, column, row) >> std::to_underlying(BattleCell::EXIT_SHIFT)) - 1)
                  == direction
              && pick(column, row, 30))
            return true;
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
  Byte(ds, x) = column;
  Byte(ds, y) = row;
}

uint16_t svc_btlaiunt(uint8_t* ds, uint32_t service, const uint16_t* args, unsigned int count) {
  return svc_battle_ai(ds, service, args, count);
}
