// Timed waits. A wait runs the sound driver's timer and polls the keyboard while it is busy; ESC quits to DOS, but only
// when both sides are under human control. Units are six ticks; the message pause waits three units per step of the
// message speed and ignores its byte argument (callers pass a reason code).
#include "io/waitkey.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/keys.hpp"
#include "common/memory.hpp"
#include "core/settings.hpp"
#include "core/textout.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

constexpr std::uint16_t TICKS_PER_UNIT{ 6 };
constexpr std::uint8_t  UNITS_PER_SPEED_STEP{ 3 };
} // namespace

void opeu_delay_ticks(Game& game, std::uint16_t ticks) {
  auto const& data{ Data(game) };
  opeu_fm_wait_start(ticks);
  while (opeu_fm_busy()) {
    if (opeu_turn_input_b() && static_cast<std::uint8_t>(opeu_key_read(game, static_cast<TextHost*>(port_textout_host()))) == KEY_ESCAPE && (data.side_control[0] | data.side_control[1]) == 0)
      opeu_quit_to_dos(game, svc_settings);
  }
}

void opeu_delay_units(Game& game, std::uint8_t units) {
  opeu_delay_ticks(game, static_cast<std::uint16_t>(units * TICKS_PER_UNIT));
}

void opeu_pending_finish(Game& game, std::uint8_t) {
  opeu_delay_units(game, static_cast<std::uint8_t>(Data(game).message_speed * UNITS_PER_SPEED_STEP));
}
