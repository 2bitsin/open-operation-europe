// The player's battle exit check for an element at an edge cell. An edge cell's battle cell holds its exit direction;
// a unit not next to the battle's cell (BATTLE_NOT_ADJACENT) needs a free, passable strategic cell in that direction
// that is not the enemy's; otherwise the cell's direction must be the unit's own. The player's twin of
// opeu_battle_ai_entry_ok.
#include "map/btlexit.hpp"
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
#include "map/btlcmds.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t DIRECTIONS{ 8 };
constexpr uint8_t LAST_CELL{ BATTLE_WIDTH - 1 };
constexpr uint8_t IMPASSABLE{ 0xff }; // a move_costs entry
constexpr uint8_t MOVE_COLUMNS{ 5 };  // move_costs: a row per terrain
// battle_exit_direction when either way is open.
constexpr uint8_t ANY_SIDE{ 0xff };
} // namespace

uint16_t opeu_battle_exit_ok(uint8_t* ds, BattleEvent event, uint8_t x, uint8_t y, uint16_t element) {
  auto& data{ Data(ds) };
  if (opeu_battle_cell_occupied(ds, event, x, y)) return 0;
  auto exit{ static_cast<uint8_t>(BattleCellAt(data, x, y) >> std::to_underlying(BattleCell::EXIT_SHIFT)) };
  if (exit == 0 || exit > DIRECTIONS) return 0;
  if (x != 0 && y != 0 && x != LAST_CELL && y != LAST_CELL) return 0;
  --exit;
  auto const unit{ opeu_battle_element_unit(ds, event, element) };
  auto const direction{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_UNIT_DIRECTION, unit)) };
  if (direction != BATTLE_NOT_ADJACENT) return direction == exit;
  auto cell_x{ data.battle_x };
  auto cell_y{ data.battle_y };
  if (!opeu_grid_step(&cell_x, &cell_y, exit)) return 0;
  if (Call(ds, event, BattleRoutine::FIND_UNIT_AT, cell_x, cell_y)) return 0;
  if (auto const objective{ Call(ds, event, BattleRoutine::FIND_OBJECTIVE_AT, cell_x, cell_y) }) {
    auto const owner{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::OBJECTIVE_SIDE, objective)) };
    if (owner != opeu_battle_unit_side(ds, event, BattleList(data, 1, 0))) return 0;
  }
  for (auto const& member : Elements(data, Record<Unit>(data, unit))) {
    auto const terrain{ MapCellAt(data, cell_x, cell_y) & Bits(MapCell::TERRAIN_MASK) };
    auto const category{ data.element_types[member.type].category };
    if (data.move_costs[terrain * MOVE_COLUMNS + data.battle_move_class[category]] == IMPASSABLE) return 0;
  }
  if (MapCellAt(data, cell_x, cell_y) & Bits(MapCell::MARK_MASK)) {
    auto const enemy_mark{ opeu_battle_unit_side(ds, event, unit) ? MapCell::MARK_1 : MapCell::MARK_0 };
    if (MapCellAt(data, cell_x, cell_y) & Bits(enemy_mark)) return 0;
  }
  if (unit != Near(data, data.battle_garrison) && data.battle_exit_direction[0] != exit
      && data.battle_exit_direction[0] != ANY_SIDE)
    return 0;
  return 1;
}

uint16_t opeu_battle_cursor_exit_ok(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto& data{ Data(ds) };
  return Call(ds, event, BattleRoutine::BATTLE_CAN_ENTER, data.battle_cursor_x, data.battle_cursor_y, element)
         && opeu_battle_exit_ok(ds, event, data.battle_cursor_x, data.battle_cursor_y, element);
}

// The services of this module: the player orders group's (map/btlcmds).
uint16_t svc_btlexit(uint8_t* ds, uint32_t service, const uint16_t* args, unsigned int count) {
  return svc_battle_orders(ds, service, args, count);
}
