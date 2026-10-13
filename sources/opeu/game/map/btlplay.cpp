// The player's battle turn: the orders loop, the test for a player's side and the redraw of a side's view.
#include "map/btlplay.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "map/btlcell.hpp"
#include "map/btlcmds.hpp"
#include "map/btldraw.hpp"
#include "map/btlelem.hpp"
#include "map/btllist.hpp"
#include "map/btlmenu.hpp"
#include "map/btlpanel.hpp"
#include "map/btlsel.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t REDRAW_ALL{ 2 };
constexpr uint8_t DISPATCH_ORDERS{ 8 };

void Refresh(Game& game, BattleEvent event) {
  Call(game, event, BattleRoutine::TEXT_PAPER_SET, 0);
  Call(game, event, BattleRoutine::BATTLE_PANEL_SIDE1);
  Call(game, event, BattleRoutine::BATTLE_PANEL_SIDE0);
}
} // namespace

// The active side picks cells until it declares its orders finished: its own elements open the orders menu (or,
// before their group joins the battle, the panel and "preparing"); a spotted enemy shows its panel.
void opeu_battle_player_orders(Game& game, BattleEvent event) {
  auto& data{ Data(game) };
  opeu_battle_dispatch(game, event, DISPATCH_ORDERS);
  // After a panel the original re-enters its loop past its own prompt with the flag set.
  uint16_t prompt{ 1 };
  for (;;) {
    Call(game, event, BattleRoutine::BATTLE_COMMANDER_PANEL, data.battle_active_side, prompt);
    prompt = 1;
    auto const side{ data.battle_active_side };
    Call(game, event, BattleRoutine::CURSOR_PRINT, Near(data, data.battle_fmt_side_orders), SideColorCode(side), data.side_names[side]);
    if (!opeu_battle_select_cell(game, event)) {
      Call(game, event, BattleRoutine::BATTLE_COMMANDER_PANEL, data.battle_active_side, 0);
      auto const finished{ Call(game, event, BattleRoutine::PROMPT_YES_NO, Near(data, data.battle_msg_orders_finished)) };
      Refresh(game, event);
      if (finished) return;
      continue;
    }
    auto const element{ Call(game, event, BattleRoutine::BATTLE_ELEMENT_AT, data.battle_cursor_x, data.battle_cursor_y) };
    if (element) {
      if (opeu_battle_element_side(game, event, element) == data.battle_active_side) {
        if (opeu_battle_level_allowed(game, event, element)) {
          if (opeu_battle_orders_menu(game, event, element)) return;
        } else {
          opeu_battle_element_panel(game, event, element, 0);
          Call(game, event, BattleRoutine::BATTLE_COMMANDER_MESSAGE, data.battle_active_side, 1, Near(data, data.battle_msg_preparing));
          prompt = 0;
          continue;
        }
      } else if (opeu_battle_element_flag2(game, event, element)) {
        opeu_battle_element_panel(game, event, element, 1);
        prompt = 0;
        continue;
      }
    }
    Refresh(game, event);
  }
}

// Makes the side of list `index` the active side; whether a player commands one of its units.
uint16_t opeu_battle_side_human(Game& game, BattleEvent event, uint8_t index) {
  auto& data{ Data(game) };
  data.battle_active_side = opeu_battle_unit_side(game, event, BattleList(data, index, 0));
  for (uint8_t slot{ 0 }; slot < BATTLE_LIST_LENGTH; ++slot)
    if (auto const unit{ BattleList(data, index, slot) }; unit && Call(game, event, BattleRoutine::BATTLE_UNIT_CONTROLLED, unit)) return 1;
  return 0;
}

// Redraws the field as list `index`'s side sees it (unspotted enemies hidden), or with REDRAW_ALL as the players
// see it.
void opeu_battle_redraw_side(Game& game, BattleEvent event, uint8_t index) {
  auto& data{ Data(game) };
  for (uint8_t y{ 0 }; y < BATTLE_WIDTH; ++y)
    for (uint8_t x{ 0 }; x < BATTLE_WIDTH; ++x) {
      if (!opeu_battle_cell_occupied(game, event, x, y)) continue;
      auto const element{ Call(game, event, BattleRoutine::BATTLE_ELEMENT_AT, x, y) };
      if (index != REDRAW_ALL) {
        if (!opeu_battle_element_flag2(game, event, element) && opeu_battle_element_side(game, event, element) != opeu_battle_unit_side(game, event, BattleList(data, index, 0))) {
          opeu_battle_draw_cell(game, event, x, y);
          continue;
        }
      } else if (data.side_status[opeu_battle_element_side(game, event, element)] != SIDE_PLAYED && !opeu_battle_element_flag2(game, event, element)) {
        continue;
      }
      opeu_battle_draw_element_at(game, event, x, y, element, opeu_battle_mode(game, event, element));
    }
}

// The players take their turns, each side a player commands in order, then the whole field is redrawn.
void opeu_battle_player_turn(Game& game, BattleEvent event) {
  for (uint8_t index{ 0 }; index < SIDE_COUNT; ++index)
    if (opeu_battle_side_human(game, event, index)) {
      opeu_battle_redraw_side(game, event, index);
      opeu_battle_player_orders(game, event);
    }
  opeu_battle_redraw_side(game, event, REDRAW_ALL);
}

// The player orders segment's services (map/btlcmds).
uint16_t svc_btlplay(Game& game, uint32_t service, const uint16_t* args, unsigned int count) {
  return svc_battle_orders(game, service, args, count);
}

auto port_routine_opeu_battle_player_turn(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_battle_player_turn(game, svc_btlplay);
  return 0;
}
