// The player's battle orders menu: where its buttons go, and the dispatch of the order chosen.
#include "map/btlmenu.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/menu.hpp"
#include "core/limits.hpp"
#include "core/textout.hpp"
#include "io/keyboard.hpp"
#include "io/mouse.hpp"
#include "map/btlcalc.hpp"
#include "map/btlcmdr.hpp"
#include "map/btlcmds.hpp"
#include "map/btlexec.hpp"
#include "map/btllist.hpp"
#include "map/btlpanel.hpp"
#include "map/btlreach.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
constexpr unsigned BUTTONS{ 5 };
constexpr uint16_t BUTTON_HEIGHT{ 24 };
// The placement's DS reserve for the mouse position words the mouse service writes.
constexpr uint16_t MOUSE_RESERVE{ 6 };
constexpr uint16_t MOUSE_X_AT{ 2 };
constexpr uint16_t MOUSE_Y_AT{ 0 };
// The menu opens below and right of the pointer, kept on the screen.
constexpr uint16_t POINTER_DX{ 40 };
constexpr uint16_t POINTER_DY{ 26 };
constexpr uint16_t LAST_COLUMN{ 0x44 };
constexpr uint16_t LAST_ROW{ 0x118 };
// Side 1's keyboard menu sits further right.
constexpr uint16_t SIDE_1_SHIFT{ 49 };
// The menu's choices; the fifth button is the withdrawal.
enum class Choice : uint16_t {
  MOVE     = 1,
  ATTACK   = 2,
  BLITZ    = 3,
  DEFEND   = 4,
  REST     = 5,
  WITHDRAW = 6,
};

void Panels(Game& game) {
  opeu_battle_panel_begin(game);
  opeu_battle_panel_side0(game);
  opeu_battle_panel_side1(game);
}
} // namespace

// With the mouse the buttons stack at the pointer; with the keyboard they take their places on the active side.
void opeu_battle_menu_place(Game& game) {
  auto& data{ Data(game) };
  if (data.input_mode) {
    ServiceLocals const locals{ MOUSE_RESERVE };
    auto const          mouse_x_at{ locals.At(MOUSE_X_AT) };
    auto const          mouse_y_at{ locals.At(MOUSE_Y_AT) };
    opeu_mouse_position(game, svc_mouse, mouse_x_at, mouse_y_at);
    auto y{ static_cast<uint16_t>(Word(game, mouse_y_at) + POINTER_DY) };
    auto x{ static_cast<uint16_t>(Word(game, mouse_x_at) + POINTER_DX) };
    x = opeu_max16(opeu_min16(x >> 3, LAST_COLUMN), 1);
    y = opeu_min16(y, LAST_ROW);
    for (unsigned i{ 0 }; i < BUTTONS; ++i) {
      data.battle_menu_items[i].x = x;
      data.battle_menu_items[i].y = static_cast<uint16_t>(BUTTON_HEIGHT * i + y);
    }
  } else {
    for (unsigned i{ 0 }; i < BUTTONS; ++i) {
      data.battle_menu_items[i].x = static_cast<uint16_t>(data.battle_menu_places[i].x + (data.battle_active_side ? SIDE_1_SHIFT : 0));
      data.battle_menu_items[i].y = data.battle_menu_places[i].y;
    }
  }
}

// The cursor starts on the element's goal (its target's cell when engaged); a withdrawal marks the exits first. After
// the order, a unit not next to the battle's cell takes the exit direction of its first withdrawing element.
uint16_t opeu_battle_orders_menu(Game& game, uint16_t element) {
  auto&      data{ Data(game) };
  auto const unit{ opeu_battle_element_unit(game, element) };
  opeu_battle_element_panel(game, element, 0);
  for (;;) {
    opeu_battle_menu_place(game);
    opeu_battle_commander_panel(game, static_cast<uint8_t>(data.battle_active_side), 0);
    auto choice{ opeu_prompt_value(game, svc_keyboard, Near(data, data.battle_msg_orders), 1, BUTTONS, static_cast<std::int16_t>(1), Near(data, data.battle_menu_items)) };
    if (choice == MENU_CANCELLED) {
      Panels(game);
      opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<uint8_t>(0));
      return 0;
    }
    opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<uint8_t>(0));
    if (choice == std::to_underlying(Choice::REST)) choice = std::to_underlying(Choice::WITHDRAW);
    auto const withdraw{ choice == std::to_underlying(Choice::WITHDRAW) };
    switch (static_cast<Choice>(choice)) {
    case Choice::MOVE:
    case Choice::ATTACK:
    case Choice::BLITZ:
    case Choice::WITHDRAW: {
      auto const& position{ PositionOf(data, element) };
      if (opeu_battle_engaged(game, element)) {
        auto const& target{ PositionOf(data, position.target) };
        data.battle_cursor_x = target.x;
        data.battle_cursor_y = target.y;
      } else {
        data.battle_cursor_x = position.goal_x;
        data.battle_cursor_y = position.goal_y;
      }
      if (withdraw) {
        if (opeu_battle_lone_withdrawal(game, element)) data.battle_exit_direction[0] = NO_SIDE;
        if (!opeu_battle_mark_exits(game, element)) {
          opeu_battle_commander_message(game, Near(data, data.battle_msg_cannot_withdraw), static_cast<uint8_t>(data.battle_active_side), static_cast<uint8_t>(1));
          continue;
        }
      }
      if (!opeu_battle_order_command(game, element, static_cast<uint8_t>(choice - 1))) data.battle_cursor_x = data.battle_cursor_y = 0;
      if (withdraw) opeu_battle_unmark_exits(game, element);
      break;
    }
    case Choice::DEFEND: opeu_battle_order_defend(game, element); break;
    case Choice::REST:   opeu_battle_order_rest(game, element); break;
    }
    break;
  }
  if (static_cast<uint8_t>(opeu_battle_unit_direction(game, unit)) == BATTLE_NOT_ADJACENT && unit != Near(data, data.battle_garrison)) {
    auto exit{ NO_SIDE };
    for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it)
      if (opeu_battle_state(game, it.Near()) == std::to_underlying(BattleState::WITHDRAW)) {
        auto const& position{ PositionOf(data, it.Near()) };
        exit = static_cast<uint8_t>((BattleCellAt(data, position.goal_x, position.goal_y) >> std::to_underlying(BattleCell::EXIT_SHIFT)) - 1);
        break;
      }
    data.battle_exit_direction[0] = exit;
  }
  Panels(game);
  return 0;
}
