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

void opeu_app_initialize(Game& game) {
  auto& data{ Data(game) };
  game.Machine().Display().Clear();
  LimitMouse(game, { .x = 0, .y = 0, .width = 0x27f, .height = 0x18f });
  opeu_input_flush(game);
  opeu_army_position(game, 14, 100);
  opeu_ui_text(game, INPUT_PROMPT);
  do {
    if (static_cast<uint16_t>(opeu_turn_input_a(game))) {
      data.input_mode = 1;
      return;
    }
  } while (!game.Machine().Input().KeyWaiting());
  data.input_mode = 0;
  opeu_input_flush(game);
}
