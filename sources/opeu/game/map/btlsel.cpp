// The player's battle cell selection by mouse or keypad.
//
// Both loops move the battle cursor and return 1 when a cell is chosen (left button or Enter), 0 when cancelled
// (right button, Esc or '0'). The keypad digits step the cursor along the six hex directions. opeu_battle_select_cell
// uses the mouse when the input mode is set, with the cursor status line shown.
#include "map/btlsel.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/keys.hpp"
#include "common/memory.hpp"
#include "map/btlcmds.hpp"
#include "map/btlcurs.hpp"
#include "map/btlinfo.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// The mouse loop's DS reserve for the pointer position the mouse service writes, converted in place to a cell.
constexpr uint16_t MOUSE_RESERVE{ 8 };
constexpr uint16_t MOUSE_X_AT{ 2 };
constexpr uint16_t MOUSE_Y_AT{ 0 };
// The column of a pointer outside the battle map.
constexpr int16_t OFF_MAP{ 14 };
} // namespace

uint16_t opeu_battle_mouse_select(Game& game, BattleEvent event, uint16_t show_info) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ MOUSE_RESERVE };
  auto const          x_at{ locals.At(MOUSE_X_AT) };
  auto const          y_at{ locals.At(MOUSE_Y_AT) };
  opeu_battle_cursor_draw(game, event);
  Call(game, event, BattleRoutine::INPUT_RESET);
  int16_t shown_x{ OFF_MAP };
  int16_t shown_y{ OFF_MAP };
  if (show_info) opeu_battle_cursor_info(game, event);
  Call(game, event, BattleRoutine::MOUSE_SHOW);
  bool chosen;
  for (;;) {
    if (Call(game, event, BattleRoutine::MOUSE_RIGHT)) {
      chosen = false;
      break;
    }
    if (Call(game, event, BattleRoutine::MOUSE_LEFT)) {
      chosen = true;
      break;
    }
    Call(game, event, BattleRoutine::MOUSE_POSITION, y_at, x_at);
    opeu_battle_mouse_cell(game, event, x_at, y_at);
    auto const x{ static_cast<int16_t>(Word(game, x_at)) };
    auto const y{ static_cast<int16_t>(Word(game, y_at)) };
    if (shown_x == x && shown_y == y) continue;
    if (x != OFF_MAP) {
      Call(game, event, BattleRoutine::MOUSE_HIDE);
      opeu_battle_cursor_draw(game, event);
      data.battle_cursor_x = static_cast<uint8_t>(x);
      data.battle_cursor_y = static_cast<uint8_t>(y);
      opeu_battle_cursor_draw(game, event);
      if (show_info) opeu_battle_cursor_info(game, event);
      Call(game, event, BattleRoutine::MOUSE_SHOW);
    }
    shown_x = x;
    shown_y = y;
  }
  Call(game, event, BattleRoutine::MOUSE_HIDE);
  opeu_battle_cursor_draw(game, event);
  return chosen;
}

uint16_t opeu_battle_key_select(Game& game, BattleEvent event, uint16_t show_info) {
  opeu_battle_cursor_draw(game, event);
  enum class Choice {
    PENDING,
    CHOSEN,
    CANCELLED
  };
  auto choice{ Choice::PENDING };
  while (choice == Choice::PENDING) {
    if (show_info) opeu_battle_cursor_info(game, event);
    switch (static_cast<uint8_t>(Call(game, event, BattleRoutine::KEYBOARD_KEY))) {
    case KEY_ENTER:  choice = Choice::CHOSEN; break;
    case KEY_ESCAPE:
    case '0':        choice = Choice::CANCELLED; break;
    case '1':        opeu_battle_cursor_step(game, event, 0, 1); break;
    case '2':        opeu_battle_cursor_step(game, event, 1, 1); break;
    case '3':        opeu_battle_cursor_step(game, event, 1, 0); break;
    case '4':        opeu_battle_cursor_step(game, event, -1, 1); break;
    case '6':        opeu_battle_cursor_step(game, event, 1, -1); break;
    case '7':        opeu_battle_cursor_step(game, event, -1, 0); break;
    case '8':        opeu_battle_cursor_step(game, event, -1, -1); break;
    case '9':        opeu_battle_cursor_step(game, event, 0, -1); break;
    default:         break;
    }
  }
  opeu_battle_cursor_draw(game, event);
  return choice == Choice::CHOSEN;
}

// Selects a battle cell with the mouse or the keyboard, as the input mode says.
uint16_t opeu_battle_select_cell(Game& game, BattleEvent event) {
  if (Data(game).input_mode) return opeu_battle_mouse_select(game, event, 1);
  return opeu_battle_key_select(game, event, 1);
}

// The player orders segment's services (map/btlcmds).
uint16_t svc_btlsel(Game& game, uint32_t service, const uint16_t* args, unsigned int count) {
  return svc_battle_orders(game, service, args, count);
}

auto port_routine_opeu_battle_key_select(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_battle_key_select(game, svc_btlsel, Arg(args, count, 0));
}

auto port_routine_opeu_battle_mouse_select(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_battle_mouse_select(game, svc_btlsel, Arg(args, count, 0));
}
