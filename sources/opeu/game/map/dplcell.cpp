// Deployment cells. The facing of a deployed element is towards the attacker's map cell when the element's unit
// defends, otherwise set by the unit's approach direction. An element is put at the battle cursor or lifted off the
// field; it may be placed on a free cell of its side's deployment zone (the high nibble of the battle cell), which
// the zone sweeps highlight and refresh; the manual placement moves the cursor until a valid cell is chosen.
#include "map/dplcell.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/dplauto.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
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
// Facings: towards the attacker by the step direction it lies in, and away from a unit's approach direction.
constexpr std::array<uint8_t, 8> TOWARD{ 2, 1, 3, 2, 3, 2, 0, 3 };
constexpr std::array<uint8_t, 8> AWAY{ 3, 0, 2, 3, 2, 3, 1, 2 };

[[nodiscard]] auto ApproachOf(Game& game, BattleEvent event, uint16_t element) -> uint8_t {
  auto const unit{ Call(game, event, BattleRoutine::BATTLE_ELEMENT_UNIT, element) };
  return static_cast<uint8_t>(Call(game, event, BattleRoutine::BATTLE_UNIT_DIRECTION, unit));
}

template <typename Body> void ForEachBattleCell(Body&& body) {
  for (uint8_t y{ 0 }; y < BATTLE_WIDTH; ++y)
    for (uint8_t x{ 0 }; x < BATTLE_WIDTH; ++x) body(x, y);
}
} // namespace

uint8_t opeu_deploy_facing(Game& game, BattleEvent event, uint16_t element) {
  auto&      data{ Data(game) };
  auto const approach{ ApproachOf(game, event, element) };
  if (approach == DEFENDING)
    for (uint8_t direction{ 0 }; direction < TOWARD.size(); ++direction) {
      Byte(game, STEP_X_AT) = data.battle_map_x;
      Byte(game, STEP_Y_AT) = data.battle_map_y;
      if (Call(game, event, BattleRoutine::GRID_STEP, STEP_X_AT, STEP_Y_AT, direction) && data.battle_attacker_x == Byte(game, STEP_X_AT) && data.battle_attacker_y == Byte(game, STEP_Y_AT))
        return TOWARD[direction];
    }
  // Outside 0..7 the original falls off its switch with the direction itself.
  return approach < AWAY.size() ? AWAY[approach] : approach;
}

void opeu_deploy_put(Game& game, BattleEvent event, uint16_t element) {
  auto& data{ Data(game) };
  PositionOf(Data(game), element).goal_x = data.battle_cursor_x;
  PositionOf(Data(game), element).goal_y = data.battle_cursor_y;
  PositionOf(Data(game), element).state  = Bits(BattleState::DEFEND);
  Call(game, event, BattleRoutine::BATTLE_SET_ORDER, element, 0);
  auto facing{ opeu_deploy_facing(game, event, element) };
  if (!ElementTypeOf(data, element).category) facing = 0;
  Call(game, event, BattleRoutine::BATTLE_DRAW_ELEMENT_AT, data.battle_cursor_x, data.battle_cursor_y, element, facing);
}

void opeu_deploy_lift(Game& game, BattleEvent event, uint16_t element) {
  Call(game, event, BattleRoutine::BATTLE_CLEAR_CELL, PositionOf(Data(game), element).x, PositionOf(Data(game), element).y);
  Call(game, event, BattleRoutine::BATTLE_DRAW_CELL, PositionOf(Data(game), element).x, PositionOf(Data(game), element).y);
  PositionOf(Data(game), element).x = BATTLE_UNPLACED;
  PositionOf(Data(game), element).y = BATTLE_UNPLACED;
}

uint16_t opeu_deploy_can_place(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint16_t element) {
  auto&      data{ Data(game) };
  auto const approach{ ApproachOf(game, event, element) };
  if (Call(game, event, BattleRoutine::BATTLE_CELL_OCCUPIED, x, y)) return 0;
  auto const zone{ static_cast<uint8_t>(data.battle_cells[y][x] >> 4) };
  auto const in_zone{ approach == DEFENDING ? zone == DEFENDER_ZONE : zone - approach == 1 };
  return in_zone && Call(game, event, BattleRoutine::BATTLE_CAN_ENTER, x, y, element);
}

void opeu_deploy_show_zone(Game& game, BattleEvent event, uint16_t element) {
  ForEachBattleCell([&](uint8_t x, uint8_t y) {
    if (opeu_deploy_can_place(game, event, x, y, element)) Call(game, event, BattleRoutine::BATTLE_HIGHLIGHT_CELL, x, y);
  });
}

void opeu_deploy_hide_zone(Game& game, BattleEvent event, uint16_t element) {
  ForEachBattleCell([&](uint8_t x, uint8_t y) {
    if (opeu_deploy_can_place(game, event, x, y, element)) Call(game, event, BattleRoutine::BATTLE_REFRESH_CELL, x, y);
  });
}

void opeu_deploy_draw_cursor(Game& game, BattleEvent event) {
  auto const& data{ Data(game) };
  Call(game, event, BattleRoutine::BATTLE_DRAW_CURSOR, data.battle_cursor_x, data.battle_cursor_y);
}

uint16_t opeu_deploy_input(Game& game, BattleEvent event) {
  if (Data(game).input_mode) return Call(game, event, BattleRoutine::BATTLE_MOUSE_SELECT, 1);
  return Call(game, event, BattleRoutine::BATTLE_KEY_SELECT, 1);
}

uint16_t opeu_deploy_manual(Game& game, BattleEvent event, uint16_t element) {
  auto& data{ Data(game) };
  for (;;) {
    Call(game, event, BattleRoutine::INPUT_FLUSH);
    Call(game, event, BattleRoutine::BATTLE_COMMANDER_PANEL, data.battle_active_side, 0);
    Call(game, event, BattleRoutine::UI_TEXT, Near(data, data.deploy_prompt));
    if (auto const chosen{ opeu_deploy_input(game, event) }; !chosen) return chosen;
    if (opeu_deploy_can_place(game, event, data.battle_cursor_x, data.battle_cursor_y, element)) {
      opeu_deploy_put(game, event, element);
      return 1;
    }
    Call(game, event, BattleRoutine::BATTLE_COMMANDER_MESSAGE, Near(data, data.deploy_refused), data.battle_active_side, 1);
  }
}

// Services of this module: the deployment group's (map/dplauto).
uint16_t svc_dplcell(Game& game, uint32_t service, const uint16_t* args, unsigned int count) {
  return svc_battle_deploy(game, service, args, count);
}
