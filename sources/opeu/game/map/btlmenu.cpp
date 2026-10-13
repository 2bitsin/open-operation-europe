// The player's battle orders menu: where its buttons go, and the dispatch of the order chosen.
#include "map/btlmenu.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btlcmds.hpp"
#include "map/btlexec.hpp"
#include "map/btllist.hpp"
#include "map/btlreach.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "glue.hpp"
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
// Below the menu's frame: the frame its placement and the command run on.
constexpr uint16_t MENU_FRAME{ 0x16 };
constexpr uint16_t CANCELLED{ 0xffff };
constexpr uint8_t  NO_SIDE{ 0xff };
// The menu's choices; the fifth button is the withdrawal.
enum class Choice : uint16_t {
  MOVE     = 1,
  ATTACK   = 2,
  BLITZ    = 3,
  DEFEND   = 4,
  REST     = 5,
  WITHDRAW = 6,
};

void Panels(uint8_t* ds, BattleEvent event) {
  Call(ds, event, BattleRoutine::BATTLE_PANEL_BEGIN);
  Call(ds, event, BattleRoutine::BATTLE_PANEL_SIDE0);
  Call(ds, event, BattleRoutine::BATTLE_PANEL_SIDE1);
}
} // namespace

// With the mouse the buttons stack at the pointer; with the keyboard they take their places on the active side.
void opeu_battle_menu_place(uint8_t* ds, BattleEvent event, uint16_t /*frame*/) {
  auto& data{ Data(ds) };
  if (data.input_mode) {
    ServiceLocals const locals{ MOUSE_RESERVE };
    auto const          mouse_x_at{ locals.At(MOUSE_X_AT) };
    auto const          mouse_y_at{ locals.At(MOUSE_Y_AT) };
    Call(ds, event, BattleRoutine::MOUSE_POSITION, mouse_y_at, mouse_x_at);
    auto y{ static_cast<uint16_t>(Word(ds, mouse_y_at) + POINTER_DY) };
    auto x{ static_cast<uint16_t>(Word(ds, mouse_x_at) + POINTER_DX) };
    x = Call(ds, event, BattleRoutine::MAX16, Call(ds, event, BattleRoutine::MIN16, x >> 3, LAST_COLUMN), 1);
    y = Call(ds, event, BattleRoutine::MIN16, y, LAST_ROW);
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
uint16_t opeu_battle_orders_menu(uint8_t* ds, BattleEvent event, uint16_t element, uint16_t frame) {
  auto&      data{ Data(ds) };
  auto const unit{ opeu_battle_element_unit(ds, event, element) };
  Call(ds, event, BattleRoutine::BATTLE_ELEMENT_PANEL, 0, element);
  for (;;) {
    opeu_battle_menu_place(ds, event, Local(frame, MENU_FRAME));
    Call(ds, event, BattleRoutine::BATTLE_COMMANDER_PANEL, data.battle_active_side, 0);
    auto choice{ Call(ds, event, BattleRoutine::PROMPT_VALUE, 1, BUTTONS, Near(data, data.battle_msg_orders), 1, Near(data, data.battle_menu_items)) };
    if (choice == CANCELLED) {
      Panels(ds, event);
      Call(ds, event, BattleRoutine::TEXT_PAPER_SET, 0);
      return 0;
    }
    Call(ds, event, BattleRoutine::TEXT_PAPER_SET, 0);
    if (choice == std::to_underlying(Choice::REST)) choice = std::to_underlying(Choice::WITHDRAW);
    auto const withdraw{ choice == std::to_underlying(Choice::WITHDRAW) };
    switch (static_cast<Choice>(choice)) {
    case Choice::MOVE:
    case Choice::ATTACK:
    case Choice::BLITZ:
    case Choice::WITHDRAW: {
      auto const& position{ PositionOf(data, element) };
      if (opeu_battle_engaged(ds, event, element)) {
        auto const& target{ PositionOf(data, position.target) };
        data.battle_cursor_x = target.x;
        data.battle_cursor_y = target.y;
      } else {
        data.battle_cursor_x = position.goal_x;
        data.battle_cursor_y = position.goal_y;
      }
      if (withdraw) {
        if (opeu_battle_lone_withdrawal(ds, event, element)) data.battle_exit_direction[0] = NO_SIDE;
        if (!opeu_battle_mark_exits(ds, event, element)) {
          Call(ds, event, BattleRoutine::BATTLE_COMMANDER_MESSAGE, data.battle_active_side, 1, Near(data, data.battle_msg_cannot_withdraw));
          continue;
        }
      }
      if (!opeu_battle_order_command(ds, event, element, static_cast<uint8_t>(choice - 1), Local(frame, MENU_FRAME))) data.battle_cursor_x = data.battle_cursor_y = 0;
      if (withdraw) opeu_battle_unmark_exits(ds, event, element);
      break;
    }
    case Choice::DEFEND: opeu_battle_order_defend(ds, event, element); break;
    case Choice::REST:   opeu_battle_order_rest(ds, event, element); break;
    }
    break;
  }
  if (static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_UNIT_DIRECTION, unit)) == BATTLE_NOT_ADJACENT && unit != Near(data, data.battle_garrison)) {
    auto exit{ NO_SIDE };
    for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it)
      if (opeu_battle_state(ds, event, it.Near()) == std::to_underlying(BattleState::WITHDRAW)) {
        auto const& position{ PositionOf(data, it.Near()) };
        exit = static_cast<uint8_t>((BattleCellAt(data, position.goal_x, position.goal_y) >> std::to_underlying(BattleCell::EXIT_SHIFT)) - 1);
        break;
      }
    data.battle_exit_direction[0] = exit;
  }
  Panels(ds, event);
  return 0;
}

// The services of this module: the player orders group's (map/btlcmds).
uint16_t svc_btlmenu(uint8_t* ds, uint32_t service, const uint16_t* args, unsigned int count) {
  return svc_battle_orders(ds, service, args, count);
}
