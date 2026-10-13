// The player's battle exit check for an element at an edge cell. An edge cell's battle cell holds its exit direction;
// a unit not next to the battle's cell (BATTLE_NOT_ADJACENT) needs a free, passable strategic cell in that direction
// that is not the enemy's; otherwise the cell's direction must be the unit's own. The player's twin of
// opeu_battle_ai_entry_ok.
#include "map/btlexit.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "map/btlcalc.hpp"
#include "map/btllist.hpp"
#include "map/btlunit.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "map/btlcmds.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t LAST_CELL{ BATTLE_WIDTH - 1 };
constexpr uint8_t MOVE_COLUMNS{ 5 }; // move_costs: a row per terrain
// battle_exit_direction when either way is open.
constexpr uint8_t ANY_SIDE{ 0xff };
} // namespace

uint16_t opeu_battle_exit_ok(Game& game, uint8_t x, uint8_t y, uint16_t element) {
  auto& data{ Data(game) };
  if (opeu_battle_cell_occupied(game, x, y)) return 0;
  auto exit{ static_cast<uint8_t>(BattleCellAt(data, x, y) >> std::to_underlying(BattleCell::EXIT_SHIFT)) };
  if (exit == 0 || exit > GRID_DIRECTIONS) return 0;
  if (x != 0 && y != 0 && x != LAST_CELL && y != LAST_CELL) return 0;
  --exit;
  auto const unit{ opeu_battle_element_unit(game, element) };
  auto const direction{ static_cast<uint8_t>(opeu_battle_unit_direction(game, unit)) };
  if (direction != BATTLE_NOT_ADJACENT) return direction == exit;
  auto cell_x{ data.battle_x };
  auto cell_y{ data.battle_y };
  if (!opeu_grid_step(&cell_x, &cell_y, exit)) return 0;
  if (opeu_find_unit_at(game, static_cast<uint8_t>(cell_x), static_cast<uint8_t>(cell_y))) return 0;
  if (auto const objective{ opeu_find_objective_at(game, static_cast<uint8_t>(cell_x), static_cast<uint8_t>(cell_y)) }) {
    auto const owner{ static_cast<uint8_t>(opeu_objective_side(game, objective)) };
    if (owner != opeu_battle_unit_side(game, BattleList(data, 1, 0))) return 0;
  }
  for (auto const& member : Elements(data, Record<Unit>(data, unit))) {
    auto const terrain{ MapCellAt(data, cell_x, cell_y) & Bits(MapCell::TERRAIN_MASK) };
    auto const category{ data.element_types[member.type].category };
    if (data.move_costs[terrain * MOVE_COLUMNS + data.battle_move_class[category]] == IMPASSABLE_COST) return 0;
  }
  if (MapCellAt(data, cell_x, cell_y) & Bits(MapCell::MARK_MASK)) {
    auto const enemy_mark{ opeu_battle_unit_side(game, unit) ? MapCell::MARK_1 : MapCell::MARK_0 };
    if (MapCellAt(data, cell_x, cell_y) & Bits(enemy_mark)) return 0;
  }
  if (unit != Near(data, data.battle_garrison) && data.battle_exit_direction[0] != exit && data.battle_exit_direction[0] != ANY_SIDE) return 0;
  return 1;
}

uint16_t opeu_battle_cursor_exit_ok(Game& game, uint16_t element) {
  auto& data{ Data(game) };
  return opeu_battle_can_enter(game, static_cast<uint8_t>(data.battle_cursor_x), static_cast<uint8_t>(data.battle_cursor_y), element)
         && opeu_battle_exit_ok(game, data.battle_cursor_x, data.battle_cursor_y, element);
}
