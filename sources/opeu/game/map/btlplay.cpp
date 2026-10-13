// The player's battle turn: the orders loop, the test for a player's side and the redraw of a side's view.
#include "map/btlplay.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "core/textout.hpp"
#include "io/keyboard.hpp"
#include "map/btlcalc.hpp"
#include "map/btlcell.hpp"
#include "map/btlcmdr.hpp"
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
#include <array>

using namespace opeu::game;

namespace {
constexpr uint8_t REDRAW_ALL{ 2 };
constexpr uint8_t DISPATCH_ORDERS{ 8 };

void Refresh(Game& game) {
  opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<uint8_t>(0));
  opeu_battle_panel_side1(game);
  opeu_battle_panel_side0(game);
}
} // namespace

// The active side picks cells until it declares its orders finished: its own elements open the orders menu (or,
// before their group joins the battle, the panel and "preparing"); a spotted enemy shows its panel.
void opeu_battle_player_orders(Game& game) {
  auto& data{ Data(game) };
  opeu_battle_dispatch(game, DISPATCH_ORDERS);
  // After a panel the original re-enters its loop past its own prompt with the flag set.
  uint16_t prompt{ 1 };
  for (;;) {
    opeu_battle_commander_panel(game, static_cast<uint8_t>(data.battle_active_side), prompt);
    prompt = 1;
    auto const side{ data.battle_active_side };
    opeu_cursor_print(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(Near(data, data.battle_fmt_side_orders)), static_cast<std::uint16_t>(SideColorCode(side)),
                                                          static_cast<std::uint16_t>(data.side_names[side]) });
    if (!opeu_battle_select_cell(game)) {
      opeu_battle_commander_panel(game, static_cast<uint8_t>(data.battle_active_side), 0);
      auto const finished{ static_cast<std::uint16_t>(opeu_prompt_yes_no(game, svc_keyboard, Near(data, data.battle_msg_orders_finished))) };
      Refresh(game);
      if (finished) return;
      continue;
    }
    auto const element{ opeu_battle_element_at(game, svc_btlcalc, static_cast<uint8_t>(data.battle_cursor_x), static_cast<uint8_t>(data.battle_cursor_y)) };
    if (element) {
      if (opeu_battle_element_side(game, element) == data.battle_active_side) {
        if (opeu_battle_level_allowed(game, element)) {
          if (opeu_battle_orders_menu(game, element)) return;
        } else {
          opeu_battle_element_panel(game, element, 0);
          opeu_battle_commander_message(game, Near(data, data.battle_msg_preparing), static_cast<uint8_t>(data.battle_active_side), static_cast<uint8_t>(1));
          prompt = 0;
          continue;
        }
      } else if (opeu_battle_element_flag2(game, element)) {
        opeu_battle_element_panel(game, element, 1);
        prompt = 0;
        continue;
      }
    }
    Refresh(game);
  }
}

// Makes the side of list `index` the active side; whether a player commands one of its units.
uint16_t opeu_battle_side_human(Game& game, uint8_t index) {
  auto& data{ Data(game) };
  data.battle_active_side = opeu_battle_unit_side(game, BattleList(data, index, 0));
  for (uint8_t slot{ 0 }; slot < BATTLE_LIST_LENGTH; ++slot)
    if (auto const unit{ BattleList(data, index, slot) }; unit && opeu_battle_unit_controlled(game, svc_btlcalc, unit)) return 1;
  return 0;
}

// Redraws the field as list `index`'s side sees it (unspotted enemies hidden), or with REDRAW_ALL as the players
// see it.
void opeu_battle_redraw_side(Game& game, uint8_t index) {
  auto& data{ Data(game) };
  for (uint8_t y{ 0 }; y < BATTLE_WIDTH; ++y)
    for (uint8_t x{ 0 }; x < BATTLE_WIDTH; ++x) {
      if (!opeu_battle_cell_occupied(game, x, y)) continue;
      auto const element{ opeu_battle_element_at(game, svc_btlcalc, static_cast<uint8_t>(x), static_cast<uint8_t>(y)) };
      if (index != REDRAW_ALL) {
        if (!opeu_battle_element_flag2(game, element) && opeu_battle_element_side(game, element) != opeu_battle_unit_side(game, BattleList(data, index, 0))) {
          opeu_battle_draw_cell(game, x, y);
          continue;
        }
      } else if (data.side_status[opeu_battle_element_side(game, element)] != SIDE_PLAYED && !opeu_battle_element_flag2(game, element)) {
        continue;
      }
      opeu_battle_draw_element_at(game, x, y, element, opeu_battle_mode(game, element));
    }
}

// The players take their turns, each side a player commands in order, then the whole field is redrawn.
void opeu_battle_player_turn(Game& game) {
  for (uint8_t index{ 0 }; index < SIDE_COUNT; ++index)
    if (opeu_battle_side_human(game, index)) {
      opeu_battle_redraw_side(game, index);
      opeu_battle_player_orders(game);
    }
  opeu_battle_redraw_side(game, REDRAW_ALL);
}
