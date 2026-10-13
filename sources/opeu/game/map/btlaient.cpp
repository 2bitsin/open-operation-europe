// The computer's battle elements holding their cell and entering the field from its edge. An edge cell's battle
// cell byte holds in its high nibble the strategic direction + 1 the field is entered from there; an entry also checks
// the strategic cell the unit would come from (occupants, the objective's side, impassable terrain for each element,
// the enemy's marks).
#include "map/btlaient.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "map/btllist.hpp"
#include "map/btlunit.hpp"
#include "map/stepmap.hpp"
#include "types.hpp"

#include "glue.hpp"
#include "map/btlaiact.hpp"
#include "port.hpp"
#include "services.hpp"

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

[[nodiscard]] auto UnitDirection(uint8_t* ds, BattleEvent event, uint16_t unit) -> uint8_t {
  return static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_UNIT_DIRECTION, unit));
}

[[nodiscard]] auto FindEntry(uint8_t* ds, BattleEvent event, uint8_t& x, uint8_t& y, uint16_t element) -> bool {
  auto&      data{ Data(ds) };
  auto const garrison{ Near(data, data.battle_garrison) };
  auto const unit{ opeu_battle_element_unit(ds, event, element) };
  auto const direction{ UnitDirection(ds, event, unit) };
  if (direction == BATTLE_NOT_ADJACENT && unit != garrison) {
    bool withdrawing{ false };
    for (auto& other : Elements(data, Record<Unit>(data, unit)))
      if (opeu_battle_state(ds, event, Near(data, other)) == Bits(BattleState::WITHDRAW)) {
        withdrawing = true;
        break;
      }
    if (!withdrawing) data.battle_ai_entry_side[0] = NONE;
  }
  uint16_t best{ NO_DISTANCE };
  uint8_t  best_x{ 0 };
  uint8_t  best_y{ 0 };
  for (uint8_t j{ 0 }; j < BATTLE_WIDTH; ++j)
    for (uint8_t i{ 0 }; i < BATTLE_WIDTH; ++i) {
      if (!Call(ds, event, BattleRoutine::BATTLE_CAN_ENTER, i, j, element)) continue;
      if (!opeu_battle_ai_entry_ok(ds, event, i, j, direction)) continue;
      auto const from_x{ PositionOf(data, element).x };
      auto const from_y{ PositionOf(data, element).y };
      if (!Call(ds, event, BattleRoutine::BATTLE_PATH_FLOOD, i, j, from_x, from_y, element, 1)) continue;
      auto const distance{ Call(ds, event, BattleRoutine::BATTLE_PATH_STEP, from_x, from_y) };
      if (distance < best) {
        best   = distance;
        best_x = i;
        best_y = j;
      }
    }
  if (best == NO_DISTANCE) return false;
  x = best_x;
  y = best_y;
  return true;
}
} // namespace

void opeu_battle_ai_hold(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto& position{ PositionOf(Data(ds), element) };
  position.goal_x = position.x;
  position.goal_y = position.y;
  position.state  = Bits(BattleState::DEFEND);
  opeu_battle_disengage(ds, event, element);
  opeu_battle_set_facing(ds, event, element, NONE);
}

uint16_t opeu_battle_ai_entry_ok(uint8_t* ds, BattleEvent event, uint8_t x, uint8_t y, uint8_t direction) {
  auto& data{ Data(ds) };
  if (opeu_battle_cell_occupied(ds, event, x, y)) return 0;
  auto side{ EntrySide(data, x, y) };
  if (side == 0 || side >= 9) return 0;
  if (x != 0 && y != 0 && x != BATTLE_EDGE && y != BATTLE_EDGE) return 0;
  --side;
  if (direction != BATTLE_NOT_ADJACENT) return direction == side;
  uint8_t cell_x{ data.battle_x };
  uint8_t cell_y{ data.battle_y };
  if (!opeu_grid_step(&cell_x, &cell_y, side)) return 0;
  if (Call(ds, event, BattleRoutine::FIND_UNIT_AT, cell_x, cell_y)) return 0;
  auto const unit{ BattleList(data, data.battle_ai_side, data.battle_ai_index) };
  auto const objective{ Call(ds, event, BattleRoutine::FIND_OBJECTIVE_AT, cell_x, cell_y) };
  if (objective) {
    auto const holder{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::OBJECTIVE_SIDE, objective)) };
    if (holder != opeu_battle_unit_side(ds, event, BattleList(data, 1, 0))) return 0;
  }
  for (auto& element : Elements(data, Record<Unit>(data, unit))) {
    auto const terrain{ static_cast<uint8_t>(MapCellAt(data, cell_x, cell_y) & Bits(MapCell::TERRAIN_MASK)) };
    auto const category{ data.element_types[element.type].category };
    auto const cost{ Record<uint8_t>(data, static_cast<uint16_t>(Near(data, data.move_costs) + terrain * MOVE_CLASSES + data.battle_ai_move_class[category])) };
    if (cost == IMPASSABLE) return 0;
  }
  if (MapCellAt(data, cell_x, cell_y) & Bits(MapCell::MARK_MASK)) {
    auto const mark{ Bits(opeu_battle_unit_side(ds, event, unit) ? MapCell::MARK_1 : MapCell::MARK_0) };
    if (MapCellAt(data, cell_x, cell_y) & mark) return 0;
  }
  if (unit != Near(data, data.battle_garrison) && data.battle_ai_entry_side[0] != side && data.battle_ai_entry_side[0] != NONE) return 0;
  return 1;
}

uint16_t opeu_battle_ai_find_entry(uint8_t* ds, BattleEvent event, uint16_t x, uint16_t y, uint16_t element) {
  uint8_t found_x{ 0 };
  uint8_t found_y{ 0 };
  if (!FindEntry(ds, event, found_x, found_y, element)) return 0;
  Byte(ds, x) = found_x;
  Byte(ds, y) = found_y;
  return 1;
}

uint16_t opeu_battle_ai_enter(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto&   data{ Data(ds) };
  uint8_t x;
  uint8_t y;
  if (!FindEntry(ds, event, x, y, element)) return 0;
  auto& position{ PositionOf(data, element) };
  position.state  = Bits(BattleState::WITHDRAW);
  position.goal_x = x;
  position.goal_y = y;
  opeu_battle_disengage(ds, event, element);
  opeu_battle_set_facing(ds, event, element, NONE);
  auto const unit{ opeu_battle_element_unit(ds, event, element) };
  if (UnitDirection(ds, event, unit) == BATTLE_NOT_ADJACENT && unit != Near(data, data.battle_garrison)) data.battle_ai_entry_side[0] = static_cast<uint8_t>(EntrySide(data, x, y) - 1);
  return 1;
}

uint16_t svc_btlaient(uint8_t* ds, uint32_t service, const uint16_t* args, unsigned int count) {
  return svc_battle_ai(ds, service, args, count);
}
