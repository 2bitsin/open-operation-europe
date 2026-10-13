// The startup stub and the first-run input-device prompt: the first key press chooses the keyboard, the first mouse
// button the mouse.
#include "core/appinit.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/startup.hpp"
#include "core/textout.hpp"
#include "io/mouse.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// Services leave the module through event. KEY_POLL reports a mouse button and MOUSE_POLL a waiting key: the prompt
// treats them the other way round, so a mouse click selects the keyboard (input mode 1).
enum Service : std::uint16_t {
  STARTUP      = 0,
  VGA_CLEAR    = 1,
  MOUSE_WINDOW = 2, // (x, y, width, height)
  INPUT_FLUSH  = 3,
  POSITION     = 4, // (x, y)
  TEXT         = 5, // (text)
  KEY_POLL     = 6,
  MOUSE_POLL   = 7,
};

constexpr std::uint16_t INPUT_PROMPT{ offsetof(DataSegment, input_prompt) };
} // namespace

void opeu_app_startup(Game& game, HostEvent event) {
  Call(game, event, STARTUP);
}

// Two empty routines nothing calls.
void opeu_empty_routine_a(void) { }

void opeu_empty_routine_b(void) { }

void opeu_app_initialize(Game& game, HostEvent event) {
  auto& data{ Data(game) };
  Call(game, event, VGA_CLEAR);
  Call(game, event, MOUSE_WINDOW, 0, 0, 0x27f, 0x18f);
  Call(game, event, INPUT_FLUSH);
  Call(game, event, POSITION, 14, 100);
  Call(game, event, TEXT, INPUT_PROMPT);
  do {
    if (Call(game, event, KEY_POLL)) {
      data.input_mode = 1;
      return;
    }
  } while (!Call(game, event, MOUSE_POLL));
  data.input_mode = 0;
  Call(game, event, INPUT_FLUSH);
}

// The services of the startup and the input prompt as direct calls.
uint16_t svc_appinit(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  switch (id) {
  case STARTUP:      opeu_app_startup_service(game, svc_startup); break;
  case VGA_CLEAR:    opeu_vga_clear(); break;
  case MOUSE_WINDOW: opeu_mouse_window(game, svc_mouse, (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1), (int16_t)Arg(args, count, 2), (int16_t)Arg(args, count, 3)); break;
  case INPUT_FLUSH:  opeu_input_flush(game, (TextHost*)port_textout_host()); break;
  case POSITION:     opeu_army_position(game, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case TEXT:         opeu_ui_text(game, (TextHost*)port_textout_host(), Arg(args, count, 0)); break;
  case KEY_POLL:     return (uint16_t)opeu_turn_input_a(game, svc_mouse);
  case MOUSE_POLL:   return opeu_turn_input_b();
  default:           port_unhandled("appinit", id, count);
  }
  return 0;
}
