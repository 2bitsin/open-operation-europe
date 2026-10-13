// The computer's battle elements holding their cell and entering the field from its edge. An edge cell's battle
// cell byte holds in its high nibble the strategic direction + 1 the field is entered from there; an entry also checks
// the strategic cell the unit would come from (occupants, the objective's side, impassable terrain for each element,
// the enemy's marks).
#include "map/btlaient.hpp"
#include "common/battle-calls.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "map/btlcalc.hpp"
#include "map/btllist.hpp"
#include "map/btlpath.hpp"
#include "map/btlunit.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <optional>
#include <ranges>

using namespace opeu::game;

namespace {
constexpr uint8_t  NONE{ 0xff };
constexpr uint16_t NO_DISTANCE{ 0xffff };
constexpr uint8_t  BATTLE_EDGE{ BATTLE_WIDTH - 1 };
constexpr uint8_t  MOVE_CLASSES{ 5 }; // move_costs entries per terrain

// The strategic direction + 1 the field is entered from at an edge cell; 0 elsewhere.
[[nodiscard]] auto EntrySide(DataSegment& data, uint8_t x, uint8_t y) -> uint8_t {
  return static_cast<uint8_t>(BattleCellAt(data, x, y) >> std::to_underlying(BattleCell::EXIT_SHIFT));
}

// Forgets the side the unit entered from when none of its elements is withdrawing any more.
void ForgetEntrySide(Game& game, uint16_t unit) {
  auto&      data{ Data(game) };
  auto const withdrawing{ [&](Element const& other) { return opeu_battle_state(game, Near(data, other)) == Bits(BattleState::WITHDRAW); } };
  if (!std::ranges::any_of(Elements(data, Record<Unit>(data, unit)), withdrawing)) data.battle_ai_entry_side[0] = NONE;
}

// The edge cell the element can withdraw to by the shortest path, if any.
[[nodiscard]] auto FindEntry(Game& game, uint16_t element) -> std::optional<GridCell> {
  auto&      data{ Data(game) };
  auto const unit{ opeu_battle_element_unit(game, element) };
  auto const direction{ UnitDirection(game, unit) };
  if (direction == BATTLE_NOT_ADJACENT && unit != Near(data, data.battle_garrison)) ForgetEntrySide(game, unit);
  auto const distance{ [&](GridCell cell) -> uint16_t {
    if (!opeu_battle_can_enter(game, cell.x, cell.y, element) || !opeu_battle_ai_entry_ok(game, cell, direction)) return NO_DISTANCE;
    auto const& from{ PositionOf(data, element) };
    if (!opeu_battle_path_flood(game, cell.x, cell.y, from.x, from.y, element, 1)) return NO_DISTANCE;
    return DownhillStep(game, { from.x, from.y }).cost;
  } };
  uint16_t                best{ NO_DISTANCE };
  std::optional<GridCell> found;
  for (auto const y : std::views::iota(uint8_t{ 0 }, BATTLE_WIDTH))
    for (auto const x : std::views::iota(uint8_t{ 0 }, BATTLE_WIDTH))
      if (auto const value{ distance({ x, y }) }; value < best) {
        best  = value;
        found = GridCell{ x, y };
      }
  return found;
}
} // namespace

void opeu_battle_ai_hold(Game& game, uint16_t element) {
  auto& position{ PositionOf(Data(game), element) };
  position.goal_x = position.x;
  position.goal_y = position.y;
  position.state  = Bits(BattleState::DEFEND);
  opeu_battle_disengage(game, element);
  opeu_battle_set_facing(game, element, NONE);
}

auto opeu_battle_ai_entry_ok(Game& game, GridCell cell, uint8_t direction) -> uint16_t {
  auto& data{ Data(game) };
  if (opeu_battle_cell_occupied(game, cell.x, cell.y)) return 0;
  auto side{ EntrySide(data, cell.x, cell.y) };
  if (side == 0 || side >= 9) return 0;
  if (cell.x != 0 && cell.y != 0 && cell.x != BATTLE_EDGE && cell.y != BATTLE_EDGE) return 0;
  --side;
  if (direction != BATTLE_NOT_ADJACENT) return direction == side;
  auto const outside{ StepOnMap({ data.battle_x, data.battle_y }, side) };
  if (!outside) return 0;
  if (opeu_find_unit_at(game, outside->x, outside->y)) return 0;
  auto const unit{ BattleList(data, data.battle_ai_side, data.battle_ai_index) };
  auto const objective{ opeu_find_objective_at(game, static_cast<uint8_t>(outside->x), static_cast<uint8_t>(outside->y)) };
  if (objective) {
    auto const holder{ static_cast<uint8_t>(opeu_objective_side(game, objective)) };
    if (holder != opeu_battle_unit_side(game, BattleList(data, 1, 0))) return 0;
  }
  for (auto& element : Elements(data, Record<Unit>(data, unit))) {
    auto const terrain{ static_cast<uint8_t>(MapCellAt(data, outside->x, outside->y) & Bits(MapCell::TERRAIN_MASK)) };
    auto const category{ data.element_types[element.type].category };
    auto const cost{ Record<uint8_t>(data, static_cast<uint16_t>(Near(data, data.move_costs) + terrain * MOVE_CLASSES + data.battle_ai_move_class[category])) };
    if (cost == IMPASSABLE_COST) return 0;
  }
  if (MapCellAt(data, outside->x, outside->y) & Bits(MapCell::MARK_MASK)) {
    auto const mark{ Bits(opeu_battle_unit_side(game, unit) ? MapCell::MARK_1 : MapCell::MARK_0) };
    if (MapCellAt(data, outside->x, outside->y) & mark) return 0;
  }
  if (unit != Near(data, data.battle_garrison) && data.battle_ai_entry_side[0] != side && data.battle_ai_entry_side[0] != NONE) return 0;
  return 1;
}

uint16_t opeu_battle_ai_enter(Game& game, uint16_t element) {
  auto&      data{ Data(game) };
  auto const entry{ FindEntry(game, element) };
  if (!entry) return 0;
  auto& position{ PositionOf(data, element) };
  position.state  = Bits(BattleState::WITHDRAW);
  position.goal_x = entry->x;
  position.goal_y = entry->y;
  opeu_battle_disengage(game, element);
  opeu_battle_set_facing(game, element, NONE);
  auto const unit{ opeu_battle_element_unit(game, element) };
  if (UnitDirection(game, unit) == BATTLE_NOT_ADJACENT && unit != Near(data, data.battle_garrison)) data.battle_ai_entry_side[0] = static_cast<uint8_t>(EntrySide(data, entry->x, entry->y) - 1);
  return 1;
}
