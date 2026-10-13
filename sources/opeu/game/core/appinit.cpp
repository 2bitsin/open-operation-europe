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

constexpr std::uint16_t INPUT_PROMPT{ offsetof(DataSegment, input_prompt) };
} // namespace

void opeu_app_startup(Game& game) {
  opeu_app_startup_service(game);
}

// Two empty routines nothing calls.
void opeu_empty_routine_a(void) { }

void opeu_empty_routine_b(void) { }

void opeu_app_initialize(Game& game) {
  auto& data{ Data(game) };
  opeu_vga_clear();
  opeu_mouse_window(game, svc_mouse, static_cast<int16_t>(0), static_cast<int16_t>(0), static_cast<int16_t>(0x27f), static_cast<int16_t>(0x18f));
  opeu_input_flush(game, static_cast<TextHost*>(port_textout_host()));
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(14), static_cast<int16_t>(100));
  opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), INPUT_PROMPT);
  do {
    if (static_cast<uint16_t>(opeu_turn_input_a(game, svc_mouse))) {
      data.input_mode = 1;
      return;
    }
  } while (!opeu_turn_input_b());
  data.input_mode = 0;
  opeu_input_flush(game, static_cast<TextHost*>(port_textout_host()));
}
