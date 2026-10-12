// Timed waits. A wait runs the sound driver's timer and polls the keyboard while it is busy; ESC quits to DOS, but only
// when both sides are under human control. Units are six ticks; the message pause waits three units per step of the
// message speed and ignores its byte argument (callers pass a reason code).
#include "io/waitkey.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/settings.hpp"
#include "core/textout.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

enum Service : std::uint16_t {
  TIMER_START = 0, // (ticks)
  TIMER_BUSY  = 1, // () -> word
  KEY_READY   = 2, // () -> word
  KEY_READ    = 3, // () -> AL
  QUIT        = 4, // quit to DOS
};

constexpr std::uint8_t  ESCAPE{ 0x1b };
constexpr std::uint16_t TICKS_PER_UNIT{ 6 };
constexpr std::uint8_t  UNITS_PER_SPEED_STEP{ 3 };
} // namespace

void opeu_delay_ticks(std::uint8_t* ds, HostEvent event, std::uint16_t ticks) {
  auto const& data{ Data(ds) };
  Call(ds, event, TIMER_START, ticks);
  while (Call(ds, event, TIMER_BUSY)) {
    if (Call(ds, event, KEY_READY) &&
        static_cast<std::uint8_t>(Call(ds, event, KEY_READ)) == ESCAPE &&
        (data.side_control[0] | data.side_control[1]) == 0)
      Call(ds, event, QUIT);
  }
}

void opeu_delay_units(std::uint8_t* ds, HostEvent event, std::uint8_t units) {
  opeu_delay_ticks(ds, event,
                   static_cast<std::uint16_t>(units * TICKS_PER_UNIT));
}

void opeu_pending_finish(std::uint8_t* ds, HostEvent event, std::uint8_t) {
  opeu_delay_units(
      ds, event,
      static_cast<std::uint8_t>(Data(ds).message_speed * UNITS_PER_SPEED_STEP));
}

auto svc_waitkey(std::uint8_t* ds, std::uint16_t kind,
                 std::uint16_t const* args, std::uint16_t count)
    -> std::uint16_t {
  switch (kind) {
    case TIMER_START: opeu_fm_wait_start(Arg(args, count, 0)); break;
    case TIMER_BUSY:  return opeu_fm_busy();
    case KEY_READY:   return opeu_turn_input_b();
    case KEY_READ:
      return opeu_key_read(ds, static_cast<TextHost*>(port_textout_host()));
    case QUIT: opeu_quit_to_dos(ds, svc_settings); break;
    default:   port_unhandled("waitkey", kind, count);
  }
  return 0;
}

auto port_routine_opeu_delay_units(std::uint8_t* ds, std::uint16_t const* args,
                                   unsigned count) -> std::uint32_t {
  opeu_delay_units(ds, svc_waitkey,
                   static_cast<std::uint8_t>(Arg(args, count, 0)));
  return 0;
}
