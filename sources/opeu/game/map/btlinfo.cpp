// The battle cursor's status line: the cell, its terrain and garrison share, and the element on it when the active
// side may see it.
#include "map/btlinfo.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "map/btllist.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "map/btlcmds.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// The cell terrain class of the objective's cells.
constexpr uint8_t  OBJECTIVE_TERRAIN{ 10 };
constexpr uint16_t PAPER{ 4 };
constexpr uint16_t PAPER_OFF{ 0 };
constexpr uint16_t LABEL_COLOR{ 7 };
constexpr uint16_t LINE_X{ 2 };
constexpr uint16_t CELL_LINE_Y{ 0x50 };
constexpr uint16_t TYPE_LINE_Y{ 0x68 };
constexpr uint16_t QUANTITY_LINE_Y{ 0x78 };
} // namespace

void opeu_battle_cursor_info(Game& game, BattleEvent event) {
  auto&      data{ Data(game) };
  auto const x{ data.battle_cursor_x };
  auto const y{ data.battle_cursor_y };
  auto const terrain{ static_cast<uint8_t>(Call(game, event, BattleRoutine::BATTLE_CELL_TERRAIN, x, y)) };
  Call(game, event, BattleRoutine::BATTLE_INFO_CLEAR);
  Call(game, event, BattleRoutine::TEXT_PAPER_SET, PAPER);
  Call(game, event, BattleRoutine::ARMY_POSITION, LINE_X, CELL_LINE_Y);
  Call(game, event, BattleRoutine::CURSOR_PRINT, Near(data, data.battle_fmt_cursor), data.battle_cursor_x, data.battle_cursor_y, data.battle_terrain_names[terrain]);
  if (terrain == OBJECTIVE_TERRAIN)
    for (unsigned i{ 0 }; i < BATTLE_EXTRA_COUNT; ++i) {
      auto const& extra{ BattleExtraAt(data, i) };
      if (extra.x == BATTLE_UNPLACED) break;
      if (extra.x == data.battle_cursor_x && extra.y == data.battle_cursor_y) {
        Call(game, event, BattleRoutine::CURSOR_PRINT, Near(data, data.battle_fmt_extra), extra.count);
        break;
      }
    }
  auto const element{ Call(game, event, BattleRoutine::BATTLE_ELEMENT_AT, data.battle_cursor_x, data.battle_cursor_y) };
  if (element && (opeu_battle_element_side(game, event, element) == data.battle_active_side || opeu_battle_element_flag2(game, event, element))) {
    Call(game, event, BattleRoutine::TEXT_COLOR, SideTextColor(opeu_battle_element_side(game, event, element)));
    Call(game, event, BattleRoutine::ARMY_POSITION, LINE_X, TYPE_LINE_Y);
    auto const& record{ Record<Element>(data, element) };
    Call(game, event, BattleRoutine::UI_TEXT, data.element_types[record.type].name);
    Call(game, event, BattleRoutine::TEXT_COLOR, LABEL_COLOR);
    Call(game, event, BattleRoutine::ARMY_POSITION, LINE_X, QUANTITY_LINE_Y);
    Call(game, event, BattleRoutine::CURSOR_PRINT, Near(data, data.battle_fmt_cell_ok), record.quantity);
  }
  Call(game, event, BattleRoutine::TEXT_PAPER_SET, PAPER_OFF);
}

// The services of this module: the player orders group's (map/btlcmds).
uint16_t svc_btlinfo(Game& game, uint32_t service, const uint16_t* args, unsigned int count) {
  return svc_battle_orders(game, service, args, count);
}
