// The player's battle turn: the orders loop, the test for a player's side and the redraw of a side's view.
#include "types.hpp"
#include "map/btlplay.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "map/btlcell.hpp"
#include "map/btlcmds.hpp"
#include "map/btldraw.hpp"
#include "map/btlelem.hpp"
#include "map/btllist.hpp"
#include "map/btlmenu.hpp"
#include "map/btlpanel.hpp"
#include "map/btlsel.hpp"
#include "map/btlunit.hpp"
#include "common/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// The frames below the orders loop's that its callees run on: the cell selection (given as its entry SP), the
// panels and the menu; the element drawing below the redraw's frame; the player turn's frame below its entry SP.
constexpr uint16_t SELECT_SP{ 6 };
constexpr uint16_t PANEL_FRAME{ 8 };
constexpr uint16_t DRAW_FRAME{ 0x12 };
constexpr uint16_t TURN_FRAME{ 6 };
constexpr uint8_t  REDRAW_ALL{ 2 };
constexpr uint8_t  DISPATCH_ORDERS{ 8 };
constexpr uint16_t SIDE_0_ESCAPE{ '6' };
constexpr uint16_t SIDE_1_ESCAPE{ '5' };

void Refresh(uint8_t* ds, BattleEvent event) {
  Call(ds, event, BattleRoutine::TEXT_PAPER_SET, 0);
  Call(ds, event, BattleRoutine::BATTLE_PANEL_SIDE1);
  Call(ds, event, BattleRoutine::BATTLE_PANEL_SIDE0);
}
} // namespace

// The active side picks cells until it declares its orders finished: its own elements open the orders menu (or,
// before their group joins the battle, the panel and "preparing"); a spotted enemy shows its panel. frame: the
// routine's frame on the DS stack; the selection, the panels and the menu run below it.
void opeu_battle_player_orders(uint8_t* ds, BattleEvent event, uint16_t frame) {
  auto& data{ Data(ds) };
  opeu_battle_dispatch(ds, event, DISPATCH_ORDERS);
  // After a panel the original re-enters its loop past its own prompt with the flag set.
  uint16_t prompt{ 1 };
  for (;;) {
    Call(ds, event, BattleRoutine::BATTLE_COMMANDER_PANEL,
         data.battle_active_side, prompt);
    prompt = 1;
    auto const side{ data.battle_active_side };
    Call(ds, event, BattleRoutine::CURSOR_PRINT,
         Near(data, data.battle_fmt_side_orders),
         side == 0 ? SIDE_0_ESCAPE : SIDE_1_ESCAPE, data.side_names[side]);
    if (!opeu_battle_select_cell(ds, event, Local(frame, SELECT_SP))) {
      Call(ds, event, BattleRoutine::BATTLE_COMMANDER_PANEL,
           data.battle_active_side, 0);
      auto const finished{ Call(ds, event, BattleRoutine::PROMPT_YES_NO,
                                Near(data, data.battle_msg_orders_finished)) };
      Refresh(ds, event);
      if (finished)
        return;
      continue;
    }
    auto const element{ Call(ds, event, BattleRoutine::BATTLE_ELEMENT_AT,
                             data.battle_cursor_x, data.battle_cursor_y) };
    if (element) {
      if (opeu_battle_element_side(ds, event, element) ==
          data.battle_active_side) {
        if (opeu_battle_level_allowed(ds, event, element)) {
          if (opeu_battle_orders_menu(ds, event, element,
                                      Local(frame, PANEL_FRAME)))
            return;
        } else {
          opeu_battle_element_panel(ds, event, element, 0,
                                    Local(frame, PANEL_FRAME));
          Call(ds, event, BattleRoutine::BATTLE_COMMANDER_MESSAGE,
               data.battle_active_side, 1,
               Near(data, data.battle_msg_preparing));
          prompt = 0;
          continue;
        }
      } else if (opeu_battle_element_flag2(ds, event, element)) {
        opeu_battle_element_panel(ds, event, element, 1,
                                  Local(frame, PANEL_FRAME));
        prompt = 0;
        continue;
      }
    }
    Refresh(ds, event);
  }
}

// Makes the side of list `index` the active side; whether a player commands one of its units.
uint16_t opeu_battle_side_human(uint8_t* ds, BattleEvent event, uint8_t index) {
  auto& data{ Data(ds) };
  data.battle_active_side =
      opeu_battle_unit_side(ds, event, BattleList(data, index, 0));
  for (uint8_t slot{ 0 }; slot < BATTLE_LIST_LENGTH; ++slot)
    if (auto const unit{ BattleList(data, index, slot) };
        unit && Call(ds, event, BattleRoutine::BATTLE_UNIT_CONTROLLED, unit))
      return 1;
  return 0;
}

// Redraws the field as list `index`'s side sees it (unspotted enemies hidden), or with REDRAW_ALL as the players
// see it. frame: the routine's frame on the DS stack; the drawing runs below it.
void opeu_battle_redraw_side(uint8_t* ds, BattleEvent event, uint8_t index,
                             uint16_t frame) {
  auto& data{ Data(ds) };
  for (uint8_t y{ 0 }; y < BATTLE_WIDTH; ++y)
    for (uint8_t x{ 0 }; x < BATTLE_WIDTH; ++x) {
      if (!opeu_battle_cell_occupied(ds, event, x, y))
        continue;
      auto const element{ Call(ds, event, BattleRoutine::BATTLE_ELEMENT_AT, x,
                               y) };
      if (index != REDRAW_ALL) {
        if (!opeu_battle_element_flag2(ds, event, element) &&
            opeu_battle_element_side(ds, event, element) !=
                opeu_battle_unit_side(ds, event, BattleList(data, index, 0))) {
          opeu_battle_draw_cell(ds, event, x, y);
          continue;
        }
      } else if (data.side_status[opeu_battle_element_side(
                     ds, event, element)] != SIDE_PLAYED &&
                 !opeu_battle_element_flag2(ds, event, element)) {
        continue;
      }
      opeu_battle_draw_element_at(ds, event, x, y, element,
                                  opeu_battle_mode(ds, event, element),
                                  Local(frame, DRAW_FRAME));
    }
}

// sp: the entry SP on the DS stack.
void opeu_battle_player_turn(uint8_t* ds, BattleEvent event, uint16_t sp) {
  auto const frame{ Local(sp, TURN_FRAME) };
  for (uint8_t index{ 0 }; index < SIDE_COUNT; ++index)
    if (opeu_battle_side_human(ds, event, index)) {
      opeu_battle_redraw_side(ds, event, index, frame);
      opeu_battle_player_orders(ds, event, frame);
    }
  opeu_battle_redraw_side(ds, event, REDRAW_ALL, frame);
}

// The player orders segment's services (map/btlcmds).
uint16_t svc_btlplay(uint8_t* ds, uint32_t service, const uint16_t* args,
                     unsigned int count) {
  return svc_battle_orders(ds, service, args, count);
}

auto port_routine_opeu_battle_player_turn(std::uint8_t* ds,
                                          std::uint16_t const*, unsigned)
    -> std::uint32_t {
  WithBattleStack(
      [&](uint16_t sp) { opeu_battle_player_turn(ds, svc_btlplay, sp); });
  return 0;
}
