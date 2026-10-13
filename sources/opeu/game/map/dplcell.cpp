// Deployment cells. The facing of a deployed element is towards the attacker's map cell when the element's unit
// defends, otherwise set by the unit's approach direction. An element is put at the battle cursor or lifted off the
// field; it may be placed on a free cell of its side's deployment zone (the high nibble of the battle cell), which
// the zone sweeps highlight and refresh; the manual placement moves the cursor until a valid cell is chosen.
#include "map/dplcell.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/textout.hpp"
#include "map/btlcalc.hpp"
#include "map/btlcell.hpp"
#include "map/btlcmdr.hpp"
#include "map/btldraw.hpp"
#include "map/btlelem.hpp"
#include "map/btllist.hpp"
#include "map/btlsel.hpp"
#include "map/btlunit.hpp"
#include "map/dplauto.hpp"
#include "map/stepmap.hpp"
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

[[nodiscard]] auto ApproachOf(Game& game, uint16_t element) -> uint8_t {
  auto const unit{ opeu_battle_element_unit(game, element) };
  return static_cast<uint8_t>(opeu_battle_unit_direction(game, svc_btlcalc, unit));
}

template <typename Body> void ForEachBattleCell(Body&& body) {
  for (uint8_t y{ 0 }; y < BATTLE_WIDTH; ++y)
    for (uint8_t x{ 0 }; x < BATTLE_WIDTH; ++x) body(x, y);
}
} // namespace

uint8_t opeu_deploy_facing(Game& game, uint16_t element) {
  auto&      data{ Data(game) };
  auto const approach{ ApproachOf(game, element) };
  if (approach == DEFENDING)
    for (uint8_t direction{ 0 }; direction < TOWARD.size(); ++direction) {
      Byte(game, STEP_X_AT) = data.battle_map_x;
      Byte(game, STEP_Y_AT) = data.battle_map_y;
      if (opeu_grid_step(game.State().Bytes(STEP_X_AT, 1).data(), game.State().Bytes(STEP_Y_AT, 1).data(), static_cast<uint8_t>(direction)) && data.battle_attacker_x == Byte(game, STEP_X_AT)
          && data.battle_attacker_y == Byte(game, STEP_Y_AT))
        return TOWARD[direction];
    }
  // Outside 0..7 the original falls off its switch with the direction itself.
  return approach < AWAY.size() ? AWAY[approach] : approach;
}

void opeu_deploy_put(Game& game, uint16_t element) {
  auto& data{ Data(game) };
  PositionOf(Data(game), element).goal_x = data.battle_cursor_x;
  PositionOf(Data(game), element).goal_y = data.battle_cursor_y;
  PositionOf(Data(game), element).state  = Bits(BattleState::DEFEND);
  opeu_battle_set_order(game, element, static_cast<uint8_t>(0));
  auto facing{ opeu_deploy_facing(game, element) };
  if (!ElementTypeOf(data, element).category) facing = 0;
  opeu_battle_draw_element_at(game, static_cast<uint8_t>(data.battle_cursor_x), static_cast<uint8_t>(data.battle_cursor_y), element, static_cast<uint8_t>(facing));
}

void opeu_deploy_lift(Game& game, uint16_t element) {
  opeu_battle_clear_cell(game, static_cast<uint8_t>(PositionOf(Data(game), element).x), static_cast<uint8_t>(PositionOf(Data(game), element).y));
  opeu_battle_draw_cell(game, static_cast<uint8_t>(PositionOf(Data(game), element).x), static_cast<uint8_t>(PositionOf(Data(game), element).y));
  PositionOf(Data(game), element).x = BATTLE_UNPLACED;
  PositionOf(Data(game), element).y = BATTLE_UNPLACED;
}

uint16_t opeu_deploy_can_place(Game& game, uint8_t x, uint8_t y, uint16_t element) {
  auto&      data{ Data(game) };
  auto const approach{ ApproachOf(game, element) };
  if (opeu_battle_cell_occupied(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y))) return 0;
  auto const zone{ static_cast<uint8_t>(data.battle_cells[y][x] >> 4) };
  auto const in_zone{ approach == DEFENDING ? zone == DEFENDER_ZONE : zone - approach == 1 };
  return in_zone && opeu_battle_can_enter(game, svc_btlcalc, static_cast<uint8_t>(x), static_cast<uint8_t>(y), element);
}

void opeu_deploy_show_zone(Game& game, uint16_t element) {
  ForEachBattleCell([&](uint8_t x, uint8_t y) {
    if (opeu_deploy_can_place(game, x, y, element)) opeu_battle_draw_frame(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y));
  });
}

void opeu_deploy_hide_zone(Game& game, uint16_t element) {
  ForEachBattleCell([&](uint8_t x, uint8_t y) {
    if (opeu_deploy_can_place(game, x, y, element)) opeu_battle_refresh_cell(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y));
  });
}

uint16_t opeu_deploy_input(Game& game) {
  if (Data(game).input_mode) return opeu_battle_mouse_select(game, 1);
  return opeu_battle_key_select(game, 1);
}

uint16_t opeu_deploy_manual(Game& game, uint16_t element) {
  auto& data{ Data(game) };
  for (;;) {
    opeu_input_flush(game, static_cast<TextHost*>(port_textout_host()));
    opeu_battle_commander_panel(game, static_cast<uint8_t>(data.battle_active_side), 0);
    opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), Near(data, data.deploy_prompt));
    if (auto const chosen{ opeu_deploy_input(game) }; !chosen) return chosen;
    if (opeu_deploy_can_place(game, data.battle_cursor_x, data.battle_cursor_y, element)) {
      opeu_deploy_put(game, element);
      return 1;
    }
    opeu_battle_commander_message(game, Near(data, data.deploy_refused), static_cast<uint8_t>(data.battle_active_side), static_cast<uint8_t>(1));
  }
}
