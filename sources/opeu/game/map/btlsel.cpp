// The player's battle cell selection by mouse or keypad.
//
// Both loops move the battle cursor and return 1 when a cell is chosen (left button or Enter), 0 when cancelled
// (right button, Esc or '0'). The keypad digits step the cursor along the six hex directions. opeu_battle_select_cell
// uses the mouse when the input mode is set, with the cursor status line shown.
#include "types.hpp"
#include "map/btlsel.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "map/btlcmds.hpp"
#include "map/btlcurs.hpp"
#include "map/btlinfo.hpp"
#include "common/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// Below the mouse loop's frame: the pointer position the mouse service writes, converted in place to a cell.
constexpr uint16_t MOUSE_X_AT{ 6 };
constexpr uint16_t MOUSE_Y_AT{ 8 };
// The mouse loop's frame below select_cell's entry SP.
constexpr uint16_t MOUSE_FRAME{ 6 };
// The column of a pointer outside the battle map.
constexpr int16_t OFF_MAP{ 14 };
constexpr uint8_t KEY_ENTER{ 0x0d };
constexpr uint8_t KEY_ESCAPE{ 0x1b };
} // namespace

// frame: the routine's frame on the DS stack; the pointer position lives there.
uint16_t opeu_battle_mouse_select(uint8_t* ds, BattleEvent event,
                                  uint16_t show_info, uint16_t frame) {
  auto&      data{ Data(ds) };
  auto const x_at{ Local(frame, MOUSE_X_AT) };
  auto const y_at{ Local(frame, MOUSE_Y_AT) };
  opeu_battle_cursor_draw(ds, event);
  Call(ds, event, BattleRoutine::INPUT_RESET);
  int16_t shown_x{ OFF_MAP };
  int16_t shown_y{ OFF_MAP };
  if (show_info)
    opeu_battle_cursor_info(ds, event);
  Call(ds, event, BattleRoutine::MOUSE_SHOW);
  bool chosen;
  for (;;) {
    if (Call(ds, event, BattleRoutine::MOUSE_RIGHT)) {
      chosen = false;
      break;
    }
    if (Call(ds, event, BattleRoutine::MOUSE_LEFT)) {
      chosen = true;
      break;
    }
    Call(ds, event, BattleRoutine::MOUSE_POSITION, y_at, x_at);
    opeu_battle_mouse_cell(ds, event, x_at, y_at);
    auto const x{ static_cast<int16_t>(Word(ds, x_at)) };
    auto const y{ static_cast<int16_t>(Word(ds, y_at)) };
    if (shown_x == x && shown_y == y)
      continue;
    if (x != OFF_MAP) {
      Call(ds, event, BattleRoutine::MOUSE_HIDE);
      opeu_battle_cursor_draw(ds, event);
      data.battle_cursor_x = static_cast<uint8_t>(x);
      data.battle_cursor_y = static_cast<uint8_t>(y);
      opeu_battle_cursor_draw(ds, event);
      if (show_info)
        opeu_battle_cursor_info(ds, event);
      Call(ds, event, BattleRoutine::MOUSE_SHOW);
    }
    shown_x = x;
    shown_y = y;
  }
  Call(ds, event, BattleRoutine::MOUSE_HIDE);
  opeu_battle_cursor_draw(ds, event);
  return chosen;
}

uint16_t opeu_battle_key_select(uint8_t* ds, BattleEvent event,
                                uint16_t show_info) {
  opeu_battle_cursor_draw(ds, event);
  enum class Choice { PENDING, CHOSEN, CANCELLED };
  auto choice{ Choice::PENDING };
  while (choice == Choice::PENDING) {
    if (show_info)
      opeu_battle_cursor_info(ds, event);
    switch (
        static_cast<uint8_t>(Call(ds, event, BattleRoutine::KEYBOARD_KEY))) {
      case KEY_ENTER:  choice = Choice::CHOSEN; break;
      case KEY_ESCAPE:
      case '0':        choice = Choice::CANCELLED; break;
      case '1':        opeu_battle_cursor_step(ds, event, 0, 1); break;
      case '2':        opeu_battle_cursor_step(ds, event, 1, 1); break;
      case '3':        opeu_battle_cursor_step(ds, event, 1, 0); break;
      case '4':        opeu_battle_cursor_step(ds, event, -1, 1); break;
      case '6':        opeu_battle_cursor_step(ds, event, 1, -1); break;
      case '7':        opeu_battle_cursor_step(ds, event, -1, 0); break;
      case '8':        opeu_battle_cursor_step(ds, event, -1, -1); break;
      case '9':        opeu_battle_cursor_step(ds, event, 0, -1); break;
      default:         break;
    }
  }
  opeu_battle_cursor_draw(ds, event);
  return choice == Choice::CHOSEN;
}

// sp: the entry SP on the DS stack.
uint16_t opeu_battle_select_cell(uint8_t* ds, BattleEvent event, uint16_t sp) {
  if (Data(ds).input_mode)
    return opeu_battle_mouse_select(ds, event, 1, Local(sp, MOUSE_FRAME));
  return opeu_battle_key_select(ds, event, 1);
}

// The player orders segment's services (map/btlcmds).
uint16_t svc_btlsel(uint8_t* ds, uint32_t service, const uint16_t* args,
                    unsigned int count) {
  return svc_battle_orders(ds, service, args, count);
}

auto port_routine_opeu_battle_key_select(std::uint8_t*        ds,
                                         std::uint16_t const* args,
                                         unsigned count) -> std::uint32_t {
  return opeu_battle_key_select(ds, svc_btlsel, Arg(args, count, 0));
}

auto port_routine_opeu_battle_mouse_select(std::uint8_t*        ds,
                                           std::uint16_t const* args,
                                           unsigned count) -> std::uint32_t {
  return WithBattleStack([&](uint16_t sp) {
    return opeu_battle_mouse_select(ds, svc_btlsel, Arg(args, count, 0),
                                    BattleFrame(sp));
  });
}
