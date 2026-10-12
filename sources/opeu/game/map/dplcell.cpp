// Deployment cells. The facing of a deployed element is towards the attacker's map cell when the element's unit
// defends, otherwise set by the unit's approach direction. An element is put at the battle cursor or lifted off the
// field; it may be placed on a free cell of its side's deployment zone (the high nibble of the battle cell), which
// the zone sweeps highlight and refresh; the manual placement moves the cursor until a valid cell is chosen.
#include "types.hpp"
#include "map/dplcell.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "map/dplauto.hpp"
#include "common/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"

#include <array>

using namespace opeu::game;

namespace {
// DS scratch the grid step writes its cell into.
constexpr uint16_t STEP_X_AT{ 0xeb10 };
constexpr uint16_t STEP_Y_AT{ 0xeb11 };
// The approach direction of a unit that defends (it holds the cell the attacker comes to).
constexpr uint8_t DEFENDING{ 9 };
// The deployment zone of a defender in the battle cell's high nibble; an attacker's is its approach direction + 1.
constexpr uint8_t DEFENDER_ZONE{ 9 };
constexpr uint8_t OFF_FIELD{ 0xff };
constexpr uint8_t PLACED{ 4 };
// Facings: towards the attacker by the step direction it lies in, and away from a unit's approach direction.
constexpr std::array<uint8_t, 8> TOWARD{ 2, 1, 3, 2, 3, 2, 0, 3 };
constexpr std::array<uint8_t, 8> AWAY{ 3, 0, 2, 3, 2, 3, 1, 2 };

[[nodiscard]] auto ApproachOf(uint8_t* ds, BattleEvent event, uint16_t element)
    -> uint8_t {
  auto const unit{ Call(ds, event, BattleRoutine::BATTLE_ELEMENT_UNIT,
                        element) };
  return static_cast<uint8_t>(
      Call(ds, event, BattleRoutine::BATTLE_UNIT_DIRECTION, unit));
}

// The element's placement record (its position word points at it); re-read at each use, as its own bytes change.
[[nodiscard]] auto Placement(uint8_t* ds, uint16_t element) -> uint16_t {
  return Record<Element>(Data(ds), element).position;
}

[[nodiscard]] auto PlacementByte(uint8_t* ds, uint16_t element, uint16_t n)
    -> uint8_t& {
  return Byte(ds, static_cast<uint16_t>(Placement(ds, element) + n));
}

template <typename Body> void ForEachBattleCell(Body&& body) {
  for (uint8_t y{ 0 }; y < BATTLE_WIDTH; ++y)
    for (uint8_t x{ 0 }; x < BATTLE_WIDTH; ++x)
      body(x, y);
}
} // namespace

uint8_t opeu_deploy_facing(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto&      data{ Data(ds) };
  auto const approach{ ApproachOf(ds, event, element) };
  if (approach == DEFENDING)
    for (uint8_t direction{ 0 }; direction < TOWARD.size(); ++direction) {
      Byte(ds, STEP_X_AT) = data.battle_map_x;
      Byte(ds, STEP_Y_AT) = data.battle_map_y;
      if (Call(ds, event, BattleRoutine::GRID_STEP, STEP_X_AT, STEP_Y_AT,
               direction) &&
          data.battle_attacker_x == Byte(ds, STEP_X_AT) &&
          data.battle_attacker_y == Byte(ds, STEP_Y_AT))
        return TOWARD[direction];
    }
  // Outside 0..7 the original falls off its switch with the direction itself.
  return approach < AWAY.size() ? AWAY[approach] : approach;
}

void opeu_deploy_put(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto& data{ Data(ds) };
  PlacementByte(ds, element, 5) = data.battle_cursor_x;
  PlacementByte(ds, element, 6) = data.battle_cursor_y;
  PlacementByte(ds, element, 7) = PLACED;
  Call(ds, event, BattleRoutine::BATTLE_SET_ORDER, element, 0);
  auto facing{ opeu_deploy_facing(ds, event, element) };
  if (!data.element_types[Record<Element>(data, element).type].category)
    facing = 0;
  Call(ds, event, BattleRoutine::BATTLE_DRAW_ELEMENT_AT, data.battle_cursor_x,
       data.battle_cursor_y, element, facing);
}

void opeu_deploy_lift(uint8_t* ds, BattleEvent event, uint16_t element) {
  Call(ds, event, BattleRoutine::BATTLE_CLEAR_CELL,
       PlacementByte(ds, element, 0), PlacementByte(ds, element, 1));
  Call(ds, event, BattleRoutine::BATTLE_DRAW_CELL,
       PlacementByte(ds, element, 0), PlacementByte(ds, element, 1));
  PlacementByte(ds, element, 0) = OFF_FIELD;
  PlacementByte(ds, element, 1) = OFF_FIELD;
}

uint16_t opeu_deploy_can_place(uint8_t* ds, BattleEvent event, uint8_t x,
                               uint8_t y, uint16_t element) {
  auto&      data{ Data(ds) };
  auto const approach{ ApproachOf(ds, event, element) };
  if (Call(ds, event, BattleRoutine::BATTLE_CELL_OCCUPIED, x, y))
    return 0;
  auto const zone{ static_cast<uint8_t>(data.battle_cells[y][x] >> 4) };
  auto const in_zone{ approach == DEFENDING ? zone == DEFENDER_ZONE
                                            : zone - approach == 1 };
  return in_zone &&
         Call(ds, event, BattleRoutine::BATTLE_CAN_ENTER, x, y, element);
}

void opeu_deploy_show_zone(uint8_t* ds, BattleEvent event, uint16_t element) {
  ForEachBattleCell([&](uint8_t x, uint8_t y) {
    if (opeu_deploy_can_place(ds, event, x, y, element))
      Call(ds, event, BattleRoutine::BATTLE_HIGHLIGHT_CELL, x, y);
  });
}

void opeu_deploy_hide_zone(uint8_t* ds, BattleEvent event, uint16_t element) {
  ForEachBattleCell([&](uint8_t x, uint8_t y) {
    if (opeu_deploy_can_place(ds, event, x, y, element))
      Call(ds, event, BattleRoutine::BATTLE_REFRESH_CELL, x, y);
  });
}

void opeu_deploy_draw_cursor(uint8_t* ds, BattleEvent event) {
  auto const& data{ Data(ds) };
  Call(ds, event, BattleRoutine::BATTLE_DRAW_CURSOR, data.battle_cursor_x,
       data.battle_cursor_y);
}

uint16_t opeu_deploy_input(uint8_t* ds, BattleEvent event) {
  if (Data(ds).input_mode)
    return Call(ds, event, BattleRoutine::BATTLE_MOUSE_SELECT, 1);
  return Call(ds, event, BattleRoutine::BATTLE_KEY_SELECT, 1);
}

uint16_t opeu_deploy_manual(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto& data{ Data(ds) };
  for (;;) {
    Call(ds, event, BattleRoutine::INPUT_FLUSH);
    Call(ds, event, BattleRoutine::BATTLE_COMMANDER_PANEL,
         data.battle_active_side, 0);
    Call(ds, event, BattleRoutine::UI_TEXT, Near(data, data.deploy_prompt));
    if (auto const chosen{ opeu_deploy_input(ds, event) }; !chosen)
      return chosen;
    if (opeu_deploy_can_place(ds, event, data.battle_cursor_x,
                              data.battle_cursor_y, element)) {
      opeu_deploy_put(ds, event, element);
      return 1;
    }
    Call(ds, event, BattleRoutine::BATTLE_COMMANDER_MESSAGE,
         Near(data, data.deploy_refused), data.battle_active_side, 1);
  }
}

// Services of this module: the deployment group's (map/dplauto).
uint16_t svc_dplcell(uint8_t* ds, uint32_t service, const uint16_t* args,
                     unsigned int count) {
  return svc_battle_deploy(ds, service, args, count);
}
