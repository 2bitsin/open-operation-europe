// The rounds of a battle fought out of sight: each element's turn against a random enemy, the rounds until one side
// gives way, and the end of the battle.
#include "map/btlfight.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>

using namespace opeu::game;

namespace {
constexpr uint8_t ROUNDS{ 25 };
// What the computer turn reports: the defender (1) or the attacker (2) won.
constexpr uint8_t DEFENDER_WON{ 1 };
constexpr uint8_t ATTACKER_WON{ 2 };

[[nodiscard]] auto UnitSide(uint8_t* ds, BattleEvent event, uint16_t unit) -> uint8_t {
  return static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_UNIT_SIDE, unit));
}

[[nodiscard]] auto OnCity(DataSegment& data) -> bool {
  return (MapCellAt(data, data.battle_x, data.battle_y) & Bits(MapCell::TERRAIN_MASK)) == std::to_underlying(Terrain::CITY);
}

void Finish(uint8_t* ds, BattleEvent event) {
  Call(ds, event, BattleRoutine::BATTLE_DAILY_CONDITION);
  Call(ds, event, BattleRoutine::BATTLE_CLEAR_POSITIONS);
}
} // namespace

uint16_t opeu_battle_random_target(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto&                     data{ Data(ds) };
  std::array<uint16_t, 256> enemies{};
  std::array<uint16_t, 256> members{};
  auto const                mine{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_ELEMENT_SIDE, element)) };
  auto const                first{ UnitSide(ds, event, BattleList(data, 0, 0)) };
  unsigned const            other{ first == mine ? 1u : 0u };
  uint8_t                   n{ 0 };
  for (uint8_t i{ 0 }; BattleList(data, other, i) != 0 && i < BATTLE_LIST_LENGTH; ++i) enemies[n++] = BattleList(data, other, i);
  auto const unit{ enemies[static_cast<uint8_t>(Call(ds, event, BattleRoutine::RAND_BOUNDED, n))] };
  n = 0;
  for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it) members[n++] = it.Near();
  return members[static_cast<uint8_t>(Call(ds, event, BattleRoutine::RAND_BOUNDED, n))];
}

// The element waits its rounds, counted on its battle order, then fires at a random enemy.
void opeu_battle_element_turn(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto order{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_ORDER, element)) };
  if (PositionOf(Data(ds), element).rounds > order) {
    ++order;
    Call(ds, event, BattleRoutine::BATTLE_SET_ORDER, order, element);
  } else {
    auto const target{ opeu_battle_random_target(ds, event, element) };
    Call(ds, event, BattleRoutine::BATTLE_RESOLVE_ATTACK, target, element);
  }
}

uint16_t opeu_battle_unit_fight(uint8_t* ds, BattleEvent event, uint16_t unit) {
  auto& data{ Data(ds) };
  for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it)
    if (Call(ds, event, BattleRoutine::BATTLE_ELEMENT_ACTIVE, it.Near())) {
      opeu_battle_element_turn(ds, event, it.Near());
      if (static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_COMPUTER_TURN))) return 0;
    }
  return 1;
}

// A unit fights while its group's level allows it and it has ammunition.
uint16_t opeu_battle_round(uint8_t* ds, BattleEvent event) {
  auto& data{ Data(ds) };
  for (unsigned side{ 0 }; side < SIDE_COUNT; ++side)
    for (unsigned i{ 0 }; i < BATTLE_LIST_LENGTH; ++i) {
      auto const unit{ BattleList(data, side, i) };
      if (unit != 0 && Call(ds, event, BattleRoutine::BATTLE_LEVEL_ALLOWED, Record<Unit>(data, unit).first) && Record<Unit>(data, unit).resources[0] && !opeu_battle_unit_fight(ds, event, unit))
        return 0;
    }
  return 1;
}

uint16_t opeu_battle_rounds(uint8_t* ds, BattleEvent event) {
  for (uint8_t r{ 0 }; r < ROUNDS; ++r)
    if (!opeu_battle_round(ds, event)) return 0;
  return 1;
}

// The winner takes the battle's cell: a city changes hands unless it still holds items, and the losing attacker's
// garrison returns.
void opeu_battle_finish_turn(uint8_t* ds, BattleEvent event) {
  auto&      data{ Data(ds) };
  auto const result{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_COMPUTER_TURN)) };
  if (result == DEFENDER_WON) {
    data.battle_side = UnitSide(ds, event, BattleList(data, 0, 0));
    if (OnCity(data)) {
      auto& site{ Record<Objective>(data, opeu_find_objective_at(ds, data.battle_x, data.battle_y)) };
      for (auto const& item : site.items)
        if (item.quantity) {
          data.battle_side = NO_SIDE;
          Finish(ds, event);
          return;
        }
      site.flags ^= Bits(ObjectiveFlag::SIDE_1);
      Call(ds, event, BattleRoutine::SIDE_GROUPS_RETARGET, 0);
      Call(ds, event, BattleRoutine::SIDE_GROUPS_RETARGET, 1);
    }
  }
  if (result == ATTACKER_WON) {
    data.battle_side = UnitSide(ds, event, BattleList(data, 1, 0) == 0 ? BattleList(data, 1, 1) : BattleList(data, 1, 0));
    if (OnCity(data)) Call(ds, event, BattleRoutine::BATTLE_RETURN_GARRISON);
  }
  Finish(ds, event);
}

void opeu_battle_finish_none(uint8_t* ds, BattleEvent event) {
  auto& data{ Data(ds) };
  data.battle_side = NO_SIDE;
  if (OnCity(data)) Call(ds, event, BattleRoutine::BATTLE_RETURN_GARRISON);
  Finish(ds, event);
}

uint16_t svc_btlfight(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(ds, address, args, count);
}
