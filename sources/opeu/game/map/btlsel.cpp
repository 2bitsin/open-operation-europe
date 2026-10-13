// The player's battle cell selection by mouse or keypad.
//
// Both loops move the battle cursor and return 1 when a cell is chosen (left button or Enter), 0 when cancelled
// (right button, Esc or '0'). The keypad digits step the cursor along the six hex directions. opeu_battle_select_cell
// uses the mouse when the input mode is set, with the cursor status line shown.
#include "map/btlsel.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/keys.hpp"
#include "common/memory.hpp"
#include "io/keyboard.hpp"
#include "io/mouse.hpp"
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

uint16_t opeu_battle_mouse_select(Game& game, uint16_t show_info) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ MOUSE_RESERVE };
  auto const          x_at{ locals.At(MOUSE_X_AT) };
  auto const          y_at{ locals.At(MOUSE_Y_AT) };
  opeu_battle_cursor_draw(game);
  opeu_input_reset(game, svc_mouse);
  int16_t shown_x{ OFF_MAP };
  int16_t shown_y{ OFF_MAP };
  if (show_info) opeu_battle_cursor_info(game);
  opeu_mouse_show(game, svc_mouse);
  bool chosen;
  for (;;) {
    if (static_cast<std::uint16_t>(opeu_mouse_right(game, svc_mouse))) {
      chosen = false;
      break;
    }
    if (static_cast<std::uint16_t>(opeu_mouse_left(game, svc_mouse))) {
      chosen = true;
      break;
    }
    opeu_mouse_position(game, svc_mouse, x_at, y_at);
    opeu_battle_mouse_cell(game, x_at, y_at);
    auto const x{ static_cast<int16_t>(Word(game, x_at)) };
    auto const y{ static_cast<int16_t>(Word(game, y_at)) };
    if (shown_x == x && shown_y == y) continue;
    if (x != OFF_MAP) {
      opeu_mouse_hide(game, svc_mouse);
      opeu_battle_cursor_draw(game);
      data.battle_cursor_x = static_cast<uint8_t>(x);
      data.battle_cursor_y = static_cast<uint8_t>(y);
      opeu_battle_cursor_draw(game);
      if (show_info) opeu_battle_cursor_info(game);
      opeu_mouse_show(game, svc_mouse);
    }
    shown_x = x;
    shown_y = y;
  }
  opeu_mouse_hide(game, svc_mouse);
  opeu_battle_cursor_draw(game);
  return chosen;
}

uint16_t opeu_battle_key_select(Game& game, uint16_t show_info) {
  opeu_battle_cursor_draw(game);
  enum class Choice {
    PENDING,
    CHOSEN,
    CANCELLED
  };
  auto choice{ Choice::PENDING };
  while (choice == Choice::PENDING) {
    if (show_info) opeu_battle_cursor_info(game);
    switch (static_cast<uint8_t>(opeu_keyboard_key(game, svc_keyboard))) {
    case KEY_ENTER:  choice = Choice::CHOSEN; break;
    case KEY_ESCAPE:
    case '0':        choice = Choice::CANCELLED; break;
    case '1':        opeu_battle_cursor_step(game, 0, 1); break;
    case '2':        opeu_battle_cursor_step(game, 1, 1); break;
    case '3':        opeu_battle_cursor_step(game, 1, 0); break;
    case '4':        opeu_battle_cursor_step(game, -1, 1); break;
    case '6':        opeu_battle_cursor_step(game, 1, -1); break;
    case '7':        opeu_battle_cursor_step(game, -1, 0); break;
    case '8':        opeu_battle_cursor_step(game, -1, -1); break;
    case '9':        opeu_battle_cursor_step(game, 0, -1); break;
    default:         break;
    }
  }
  opeu_battle_cursor_draw(game);
  return choice == Choice::CHOSEN;
}

// Selects a battle cell with the mouse or the keyboard, as the input mode says.
uint16_t opeu_battle_select_cell(Game& game) {
  if (Data(game).input_mode) return opeu_battle_mouse_select(game, 1);
  return opeu_battle_key_select(game, 1);
}
