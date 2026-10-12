// The battle cursor's status line: the cell, its terrain and garrison share, and the element on it when the active
// side may see it.
#include "map/btlinfo.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btllist.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "glue.hpp"
#include "map/btlcmds.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// The cell terrain class of the objective's cells.
constexpr uint8_t  OBJECTIVE_TERRAIN{ 10 };
constexpr uint16_t PAPER{ 4 };
constexpr uint16_t PAPER_OFF{ 0 };
constexpr uint16_t SIDE_1_COLOR{ 5 };
constexpr uint16_t SIDE_0_COLOR{ 6 };
constexpr uint16_t LABEL_COLOR{ 7 };
constexpr uint16_t LINE_X{ 2 };
constexpr uint16_t CELL_LINE_Y{ 0x50 };
constexpr uint16_t TYPE_LINE_Y{ 0x68 };
constexpr uint16_t QUANTITY_LINE_Y{ 0x78 };
} // namespace

void opeu_battle_cursor_info(uint8_t* ds, BattleEvent event) {
  auto&      data{ Data(ds) };
  auto const x{ data.battle_cursor_x };
  auto const y{ data.battle_cursor_y };
  auto const terrain{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_CELL_TERRAIN, x, y)) };
  Call(ds, event, BattleRoutine::BATTLE_INFO_CLEAR);
  Call(ds, event, BattleRoutine::TEXT_PAPER_SET, PAPER);
  Call(ds, event, BattleRoutine::ARMY_POSITION, LINE_X, CELL_LINE_Y);
  Call(ds, event, BattleRoutine::CURSOR_PRINT, Near(data, data.battle_fmt_cursor), data.battle_cursor_x,
       data.battle_cursor_y, data.battle_terrain_names[terrain]);
  if (terrain == OBJECTIVE_TERRAIN)
    for (unsigned i{ 0 }; i < BATTLE_EXTRA_COUNT; ++i) {
      auto const& extra{ BattleExtraAt(data, i) };
      if (extra.x == BATTLE_UNPLACED) break;
      if (extra.x == data.battle_cursor_x && extra.y == data.battle_cursor_y) {
        Call(ds, event, BattleRoutine::CURSOR_PRINT, Near(data, data.battle_fmt_extra), extra.count);
        break;
      }
    }
  auto const element{ Call(ds, event, BattleRoutine::BATTLE_ELEMENT_AT, data.battle_cursor_x, data.battle_cursor_y) };
  if (element
      && (opeu_battle_element_side(ds, event, element) == data.battle_active_side
          || opeu_battle_element_flag2(ds, event, element))) {
    Call(ds, event, BattleRoutine::TEXT_COLOR,
         opeu_battle_element_side(ds, event, element) ? SIDE_1_COLOR : SIDE_0_COLOR);
    Call(ds, event, BattleRoutine::ARMY_POSITION, LINE_X, TYPE_LINE_Y);
    auto const& record{ Record<Element>(data, element) };
    Call(ds, event, BattleRoutine::UI_TEXT, data.element_types[record.type].name);
    Call(ds, event, BattleRoutine::TEXT_COLOR, LABEL_COLOR);
    Call(ds, event, BattleRoutine::ARMY_POSITION, LINE_X, QUANTITY_LINE_Y);
    Call(ds, event, BattleRoutine::CURSOR_PRINT, Near(data, data.battle_fmt_cell_ok), record.quantity);
  }
  Call(ds, event, BattleRoutine::TEXT_PAPER_SET, PAPER_OFF);
}

// The services of this module: the player orders group's (map/btlcmds).
uint16_t svc_btlinfo(uint8_t* ds, uint32_t service, const uint16_t* args, unsigned int count) {
  return svc_battle_orders(ds, service, args, count);
}
