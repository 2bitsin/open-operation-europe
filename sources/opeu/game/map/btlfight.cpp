// The rounds of a battle fought out of sight: each element's turn against a random enemy, the rounds until one side
// gives way, and the end of the battle.
#include "map/btlfight.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "core/objrank.hpp"
#include "map/btlhit.hpp"
#include "map/btllist.hpp"
#include "map/btlturn.hpp"
#include "map/btlunit.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <array>
#include <ranges>

using namespace opeu::game;

namespace {
constexpr uint8_t ROUNDS{ 25 };
// What the computer turn reports: the defender (1) or the attacker (2) won.
constexpr uint8_t DEFENDER_WON{ 1 };
constexpr uint8_t ATTACKER_WON{ 2 };

[[nodiscard]] auto UnitSide(Game& game, uint16_t unit) -> uint8_t {
  return static_cast<uint8_t>(opeu_battle_unit_side(game, unit));
}

void Finish(Game& game) {
  opeu_battle_daily_condition(game);
  opeu_battle_clear_positions(game);
}
} // namespace

uint16_t opeu_battle_random_target(Game& game, uint16_t element) {
  auto&                     data{ Data(game) };
  std::array<uint16_t, 256> enemies{};
  std::array<uint16_t, 256> members{};
  auto const                mine{ static_cast<uint8_t>(opeu_battle_element_side(game, element)) };
  auto const                first{ UnitSide(game, BattleList(data, 0, 0)) };
  unsigned const            other{ first == mine ? 1u : 0u };
  uint8_t                   n{ 0 };
  for (uint8_t i{ 0 }; BattleList(data, other, i) != 0 && i < BATTLE_LIST_LENGTH; ++i) enemies[n++] = BattleList(data, other, i);
  auto const unit{ enemies[static_cast<uint8_t>(game.State().RandomBounded(n))] };
  n = 0;
  for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it) members[n++] = it.Near();
  return members[static_cast<uint8_t>(game.State().RandomBounded(n))];
}

// The element waits its rounds, counted on its battle order, then fires at a random enemy.
void opeu_battle_element_turn(Game& game, uint16_t element) {
  auto order{ static_cast<uint8_t>(opeu_battle_order(game, element)) };
  if (PositionOf(Data(game), element).rounds > order) {
    ++order;
    opeu_battle_set_order(game, element, static_cast<uint8_t>(order));
  } else {
    auto const target{ opeu_battle_random_target(game, element) };
    opeu_battle_resolve_attack(game, element, target);
  }
}

uint16_t opeu_battle_unit_fight(Game& game, uint16_t unit) {
  auto& data{ Data(game) };
  for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it)
    if (opeu_battle_element_active(game, it.Near())) {
      opeu_battle_element_turn(game, it.Near());
      if (static_cast<uint8_t>(opeu_battle_computer_turn(game))) return 0;
    }
  return 1;
}

// A unit fights while its group's level allows it and it has ammunition.
uint16_t opeu_battle_round(Game& game) {
  auto& data{ Data(game) };
  for (auto const side : std::views::iota(unsigned{ 0 }, SIDE_COUNT))
    for (auto const i : std::views::iota(unsigned{ 0 }, BATTLE_LIST_LENGTH)) {
      auto const unit{ BattleList(data, side, i) };
      if (unit != 0 && opeu_battle_level_allowed(game, Record<Unit>(data, unit).first) && Record<Unit>(data, unit).resources[0] && !opeu_battle_unit_fight(game, unit)) return 0;
    }
  return 1;
}

uint16_t opeu_battle_rounds(Game& game) {
  return std::ranges::all_of(std::views::iota(uint8_t{ 0 }, ROUNDS), [&](uint8_t) { return opeu_battle_round(game) != 0; }) ? 1 : 0;
}

// The winner takes the battle's cell: a city changes hands unless it still holds items, and the losing attacker's
// garrison returns.
void opeu_battle_finish_turn(Game& game) {
  auto&      data{ Data(game) };
  auto const result{ static_cast<uint8_t>(opeu_battle_computer_turn(game)) };
  if (result == DEFENDER_WON) {
    data.battle_side = UnitSide(game, BattleList(data, 0, 0));
    if (BattleOnCity(data)) {
      auto& site{ Record<Objective>(data, opeu_find_objective_at(game, data.battle_x, data.battle_y)) };
      for (auto const& item : site.items)
        if (item.quantity) {
          data.battle_side = NO_SIDE;
          Finish(game);
          return;
        }
      site.flags ^= Bits(ObjectiveFlag::SIDE_1);
      opeu_side_groups_retarget(game, static_cast<uint8_t>(0));
      opeu_side_groups_retarget(game, static_cast<uint8_t>(1));
    }
  }
  if (result == ATTACKER_WON) {
    data.battle_side = UnitSide(game, BattleList(data, 1, 0) == 0 ? BattleList(data, 1, 1) : BattleList(data, 1, 0));
    if (BattleOnCity(data)) opeu_battle_return_garrison(game);
  }
  Finish(game);
}

void opeu_battle_finish_none(Game& game) {
  auto& data{ Data(game) };
  data.battle_side = NO_SIDE;
  if (BattleOnCity(data)) opeu_battle_return_garrison(game);
  Finish(game);
}
