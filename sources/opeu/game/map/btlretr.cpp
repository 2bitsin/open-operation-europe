// Battle retreats: the cells a retreating unit may enter, the retreat itself and the per-round morale pass.
//
// A retreat cell holds no unit, an objective only when held by the side of the second list's lead unit, terrain
// every element can cross, and no mark of the unit's side. Retreating into a cell the enemy marked costs a random
// element the bombard path of the attack resolution. A garrison that breaks damages its objective instead.
#include "map/btlretr.hpp"
#include "common/battle-calls.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "core/numeric.hpp"
#include "map/btlhit.hpp"
#include "map/btllist.hpp"
#include "map/btlmove.hpp"
#include "map/btlturn.hpp"
#include "map/cellflag.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "types.hpp"
#include "units/value.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <ranges>

using namespace opeu::game;

namespace {
// BATTLE_UNIT_DIRECTION of a unit standing on the battle cell; the battle direction before any retreat.
constexpr uint8_t AT_BATTLE{ 9 };
constexpr uint8_t NO_DIRECTION{ 0xff };
constexpr uint8_t MOVE_CLASSES{ 5 }; // move_costs columns

// The unit's slot in its division: its index in units (signed 16-bit division), low three bits.
[[nodiscard]] auto UnitSlot(uint16_t unit) -> uint8_t {
  return static_cast<uint8_t>(static_cast<int16_t>(static_cast<uint16_t>(unit - UNITS_AT)) / static_cast<int16_t>(sizeof(Unit)) & 7);
}

[[nodiscard]] auto ComputerTurn(Game& game) -> uint8_t {
  return static_cast<uint8_t>(opeu_battle_computer_turn(game));
}

} // namespace

uint16_t opeu_battle_retreat_cell_ok(Game& game, uint8_t x, uint8_t y, uint16_t unit) {
  auto& data{ Data(game) };
  if (opeu_find_unit_at(game, x, y)) return 0;
  if (auto const objective{ opeu_find_objective_at(game, x, y) }) {
    auto const side{ opeu_battle_unit_side(game, BattleList(data, 1, 0)) };
    if (side != ((Record<Objective>(data, objective).flags & Bits(ObjectiveFlag::SIDE_1)) != 0)) return 0;
  }
  auto const terrain{ MapCellAt(data, x, y) & Bits(MapCell::TERRAIN_MASK) };
  for (auto const& element : Elements(data, Record<Unit>(data, unit))) {
    auto const move_class{ data.battle_retreat_class[data.element_types[element.type].category] };
    if (data.move_costs[terrain * MOVE_CLASSES + move_class] == IMPASSABLE_COST) return 0;
  }
  if (opeu_cell_marked(game, x, y, opeu_battle_unit_side(game, unit))) return 0;
  return 1;
}

// A random element of the unit takes the bombard path of the attack resolution, and the unit's side marks the cell.
void opeu_battle_retreat_to(Game& game, uint16_t unit, uint8_t x, uint8_t y) {
  auto&      data{ Data(game) };
  auto&      record{ Record<Unit>(data, unit) };
  auto const count{ static_cast<uint16_t>(std::ranges::distance(Elements(data, record))) };
  auto const chosen{ game.State().Generator().Bounded(count) };
  auto const it{ std::ranges::next(Elements(data, record).begin(), chosen) };
  opeu_battle_resolve_attack(game, 0, it.Near());
  MapCellAt(data, x, y) |= opeu_battle_unit_side(game, unit) ? Bits(MapCell::MARK_1) : Bits(MapCell::MARK_0);
}

// side: the unit's battle list (unused).
void opeu_battle_unit_retreat(Game& game, uint16_t unit, uint8_t) {
  auto& data{ Data(game) };
  if (UnitDirection(game, unit) == AT_BATTLE && unit != GARRISON_AT) {
    auto const side{ opeu_battle_unit_side(game, unit) };
    for (auto const direction : std::views::iota(uint8_t{ 0 }, GRID_DIRECTIONS)) {
      auto const cell{ StepOnMap({ data.battle_x, data.battle_y }, direction) };
      if (!cell || !opeu_battle_retreat_cell_ok(game, cell->x, cell->y, unit)) continue;
      if (opeu_cell_marked(game, cell->x, cell->y, 1)) {
        if (opeu_cell_marked(game, cell->x, cell->y, side)) continue;
        opeu_battle_retreat_to(game, unit, cell->x, cell->y);
        return;
      }
      data.battle_direction = direction;
    }
    if (data.battle_direction != NO_DIRECTION) opeu_battle_remove_list(game, unit);
  }
  if (UnitDirection(game, unit) != AT_BATTLE) opeu_battle_remove_list(game, unit);
  if (unit == GARRISON_AT) {
    auto&      objective{ Record<Objective>(data, opeu_find_objective_at(game, data.battle_x, data.battle_y)) };
    auto const elements{ opeu_battle_element_count(game, unit) };
    auto const percent{ game.State().Generator().Bounded(static_cast<uint16_t>(elements * 10u + 0x29)) };
    AbandonObjectiveStores(objective, percent);
    opeu_battle_remove_list(game, unit);
  }
}

// Units out of ammunition may retreat; a side outnumbered more than five to one may break; supply and repair units
// retreat more readily the more retreat cells they have.
void opeu_battle_morale_check(Game& game) {
  auto&                           data{ Data(game) };
  std::array<uint8_t, SIDE_COUNT> strength{};
  for (auto const list : std::views::iota(uint8_t{ 0 }, SIDE_COUNT)) {
    strength[list] = 0;
    for (auto const slot : std::views::iota(uint8_t{ 0 }, BATTLE_LIST_LENGTH))
      if (auto const unit{ BattleList(data, list, slot) }) {
        if (!Record<Unit>(data, unit).resources[0] && game.State().Generator().Bounded(100) < 30) opeu_battle_unit_retreat(game, unit, list);
        strength[list] = static_cast<uint8_t>(strength[list] + opeu_battle_element_count(game, unit));
      }
    if (!strength[list]) return;
  }
  uint8_t odds;
  uint8_t weaker;
  if (strength[1] < strength[0]) {
    odds   = static_cast<uint8_t>(strength[0] / strength[1]);
    weaker = 1;
  } else {
    odds   = static_cast<uint8_t>(strength[1] / strength[0]);
    weaker = 0;
  }
  auto const support{ [](uint16_t unit) {
    auto const slot{ UnitSlot(unit) };
    return opeu_category_6(slot) || opeu_category_7(slot);
  } };
  if (odds > 5)
    for (auto const slot : std::views::iota(uint8_t{ 0 }, BATTLE_LIST_LENGTH))
      if (auto const unit{ BattleList(data, weaker, slot) }; unit && unit != GARRISON_AT && !support(unit) && game.State().Generator().Bounded(100) < 20) {
        opeu_battle_unit_retreat(game, unit, weaker);
        if (ComputerTurn(game)) return;
      }
  for (auto const list : std::views::iota(uint8_t{ 0 }, SIDE_COUNT))
    for (auto const slot : std::views::iota(uint8_t{ 0 }, BATTLE_LIST_LENGTH)) {
      auto const unit{ BattleList(data, list, slot) };
      if (!unit || unit == GARRISON_AT || !support(unit)) continue;
      auto const open{ [&](uint8_t direction) {
        auto const cell{ StepOnMap({ data.battle_x, data.battle_y }, direction) };
        return cell && opeu_battle_retreat_cell_ok(game, cell->x, cell->y, unit);
      } };
      auto const chance{ static_cast<uint8_t>(20 + 10 * std::ranges::count_if(std::views::iota(uint8_t{ 0 }, GRID_DIRECTIONS), open)) };
      if (game.State().Generator().Bounded(100) < chance) {
        opeu_battle_unit_retreat(game, unit, list);
        if (ComputerTurn(game)) return;
      }
    }
}
