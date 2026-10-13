// The scenario briefings: the six side initializers of the scenarios speak the opening briefing of the current side.
// The side's commander and then its staff officer read the briefing texts; one text is formatted with the player's
// name. While a target city is named, the message hook blinks a marker on its cell of the overview map.
#include "core/scenbrf.hpp"
#include "common/events.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "link/glue.hpp"
#include "map/maperase.hpp"
#include "map/mappixel.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The services the briefings call through their event.
enum Service : std::uint16_t {
  ACTIVATE        = 0, // daily activate(side)
  CONTROL_MESSAGE = 1, // control message(text, speaker)
  OVERVIEW_PIXEL  = 2, // overview pixel(x pointer, y pointer)
  INVERT          = 3, // invert(x, y, width, height, mode)
  FORMAT          = 4, // format(destination, format, person)
};

// The code segment of the briefings: the message hook points at opeu_brief_blink, offset 0 in it.
constexpr std::uint16_t BRIEFING_CODE_SEGMENT{ 0x0d9b };

// The buffer that receives the closing text formatted with the player's name.
constexpr std::uint16_t CLOSING_BYTES{ 100 };

// One side's briefing in progress: who speaks, and the closing-text buffer in the briefing's reserve.
class Briefing {
public:
  Briefing(Game& game, HostEvent event) : _game{ game }, _event{ event } { }

  auto Activate(std::uint8_t side) -> void {
    Call(_game, _event, ACTIVATE, side);
  }
  auto Commander() -> void {
    _speaker = Data(_game).initial_template4;
  }
  auto Staff() -> void {
    _speaker = Data(_game).initial_template5;
  }

  template <std::size_t N> auto Say(std::array<char, N> const& text) -> void {
    SayAt(Near(Data(_game), text));
  }
  auto SayClosing() -> void {
    SayAt(_closing.At(0));
  }

  // Formats the text with the player's name into the closing buffer.
  template <std::size_t N>
  auto Format(std::array<char, N> const& text) -> void {
    auto const& data{ Data(_game) };
    Call(_game, _event, FORMAT, _closing.At(0), Near(data, text), data.initial_person);
  }

  auto HookOn() -> void {
    Data(_game).message_hook = FarPointer{ 0, BRIEFING_CODE_SEGMENT };
  }
  auto HookOff() -> void {
    Data(_game).message_hook = FarPointer{ 0, 0 };
  }
  auto Mark(std::uint8_t x, std::uint8_t y) -> void {
    opeu_brief_mark(_game, _event, x, y);
  }

private:
  auto SayAt(std::uint16_t text) -> void {
    Call(_game, _event, CONTROL_MESSAGE, text, _speaker);
  }

  Game&               _game;
  HostEvent           _event;
  ServiceLocals const _closing{ CLOSING_BYTES };
  std::uint16_t       _speaker{};
};

} // namespace

// The message hook: inverts a 1x8 marker at the marked overview cell.
void opeu_brief_blink(Game& game, HostEvent event) {
  auto const& data{ Data(game) };
  Call(game, event, INVERT, data.brief_mark_x, data.brief_mark_y, 1, 8, 0x107);
}

// Marks an overview-map cell for the blinking hook.
void opeu_brief_mark(Game& game, HostEvent event, uint8_t x, uint8_t y) {
  auto& data{ Data(game) };
  data.brief_mark_x = x;
  data.brief_mark_y = y;
  Call(game, event, OVERVIEW_PIXEL, offsetof(DataSegment, brief_mark_x), offsetof(DataSegment, brief_mark_y));
}

// France 1940.
void opeu_brief_france(Game& game, HostEvent event) {
  Briefing    briefing{ game, event };
  auto const& data{ Data(game) };
  if (data.current_side == 0) {
    briefing.Activate(0);
    briefing.Commander();
    briefing.Say(data.brief_france_axis_1);
    briefing.Staff();
    briefing.HookOn();
    briefing.Mark(0x1d, 0x18);
    briefing.Say(data.brief_france_axis_2);
    briefing.Mark(0x01, 0x0e);
    briefing.Say(data.brief_france_axis_3);
    briefing.HookOff();
    briefing.Say(data.brief_france_axis_4);
    briefing.Say(data.brief_france_axis_5);
    briefing.Format(data.brief_france_axis_6);
    briefing.SayClosing();
  } else if (data.current_side == 1) {
    briefing.Activate(1);
    briefing.Commander();
    briefing.Say(data.brief_france_allies_1);
    briefing.Say(data.brief_france_allies_2);
    briefing.Say(data.brief_france_allies_3);
    briefing.Staff();
    briefing.HookOn();
    briefing.Mark(0x1d, 0x18);
    briefing.Say(data.brief_france_allies_4);
    briefing.Mark(0x01, 0x0e);
    briefing.Say(data.brief_france_allies_5);
    briefing.Mark(0x0a, 0x21);
    briefing.Say(data.brief_france_allies_6);
    briefing.HookOff();
    briefing.Say(data.brief_france_allies_7);
    briefing.Format(data.brief_france_allies_8);
    briefing.SayClosing();
  }
}

// North Africa.
void opeu_brief_africa(Game& game, HostEvent event) {
  Briefing    briefing{ game, event };
  auto const& data{ Data(game) };
  if (data.current_side == 0) {
    briefing.Activate(0);
    briefing.Commander();
    briefing.Say(data.brief_africa_axis_1);
    briefing.Say(data.brief_africa_axis_2);
    briefing.Staff();
    briefing.HookOn();
    briefing.Mark(0x1c, 0x04);
    briefing.Say(data.brief_africa_axis_3);
    briefing.HookOff();
    briefing.Say(data.brief_africa_axis_4);
    briefing.HookOn();
    briefing.Mark(0x21, 0x0d);
    briefing.Say(data.brief_africa_axis_5);
    briefing.Mark(0x21, 0x0a);
    briefing.Say(data.brief_africa_axis_6);
    briefing.HookOff();
    briefing.Say(data.brief_africa_axis_7);
    briefing.Format(data.brief_africa_axis_8);
    briefing.SayClosing();
  } else if (data.current_side == 1) {
    briefing.Activate(1);
    briefing.Commander();
    briefing.Say(data.brief_africa_allies_1);
    briefing.Say(data.brief_africa_allies_2);
    briefing.Say(data.brief_africa_allies_3);
    briefing.Staff();
    briefing.HookOn();
    briefing.Mark(0x1c, 0x04);
    briefing.Say(data.brief_africa_allies_4);
    briefing.Mark(0x21, 0x0a);
    briefing.Say(data.brief_africa_allies_5);
    briefing.Mark(0x21, 0x0d);
    briefing.Say(data.brief_africa_allies_6);
    briefing.HookOff();
    briefing.Say(data.brief_africa_allies_7);
    briefing.Format(data.brief_africa_allies_8);
    briefing.SayClosing();
  }
}

// Kursk (Operation Citadel).
void opeu_brief_kursk(Game& game, HostEvent event) {
  Briefing    briefing{ game, event };
  auto const& data{ Data(game) };
  if (data.current_side == 0) {
    briefing.Activate(0);
    briefing.Commander();
    briefing.Say(data.brief_kursk_axis_1);
    briefing.Say(data.brief_kursk_axis_2);
    briefing.Say(data.brief_kursk_axis_3);
    briefing.Say(data.brief_kursk_axis_4);
    briefing.Staff();
    briefing.Format(data.brief_kursk_axis_5);
    briefing.SayClosing();
    briefing.HookOn();
    briefing.Mark(0x1f, 0x24);
    briefing.Say(data.brief_kursk_axis_6);
    briefing.Mark(0x13, 0x04);
    briefing.Say(data.brief_kursk_axis_7);
    briefing.HookOff();
    briefing.Say(data.brief_kursk_axis_8);
    briefing.Say(data.brief_kursk_axis_9);
  } else if (data.current_side == 1) {
    briefing.Activate(1);
    briefing.Commander();
    briefing.Say(data.brief_kursk_allies_1);
    briefing.Say(data.brief_kursk_allies_2);
    briefing.Say(data.brief_kursk_allies_3);
    briefing.Say(data.brief_kursk_allies_4);
    briefing.Say(data.brief_kursk_allies_5);
    briefing.Staff();
    briefing.HookOn();
    briefing.Format(data.brief_kursk_allies_6);
    briefing.SayClosing();
    briefing.Say(data.brief_kursk_allies_7);
    briefing.HookOff();
    briefing.Say(data.brief_kursk_allies_8);
    briefing.Say(data.brief_kursk_allies_9);
    briefing.Say(data.brief_kursk_allies_10);
  }
}

// Normandy.
void opeu_brief_normandy(Game& game, HostEvent event) {
  Briefing    briefing{ game, event };
  auto const& data{ Data(game) };
  if (data.current_side == 0) {
    briefing.Activate(0);
    briefing.Commander();
    briefing.Say(data.brief_normandy_axis_1);
    briefing.Say(data.brief_normandy_axis_2);
    briefing.Say(data.brief_normandy_axis_3);
    briefing.Say(data.brief_normandy_axis_4);
    briefing.Staff();
    briefing.HookOn();
    briefing.Mark(0x1c, 0x0d);
    briefing.Say(data.brief_normandy_axis_5);
    briefing.HookOff();
    briefing.Say(data.brief_normandy_axis_6);
    briefing.Mark(0x0c, 0x02);
    briefing.Say(data.brief_normandy_axis_7);
    briefing.HookOff();
    briefing.Format(data.brief_normandy_axis_8);
    briefing.SayClosing();
    briefing.Say(data.brief_normandy_axis_9);
    briefing.Say(data.brief_normandy_axis_10);
  } else if (data.current_side == 1) {
    briefing.Activate(1);
    briefing.Commander();
    briefing.Say(data.brief_normandy_allies_1);
    briefing.Say(data.brief_normandy_allies_2);
    briefing.Say(data.brief_normandy_allies_3);
    briefing.Say(data.brief_normandy_allies_4);
    briefing.Staff();
    briefing.Say(data.brief_normandy_allies_5);
    briefing.HookOn();
    briefing.Mark(0x1c, 0x0d);
    briefing.Say(data.brief_normandy_allies_6);
    briefing.Mark(0x0c, 0x02);
    briefing.Say(data.brief_normandy_allies_7);
    briefing.Mark(0x12, 0x02);
    briefing.Say(data.brief_normandy_allies_8);
    briefing.HookOff();
    briefing.Format(data.brief_normandy_allies_9);
    briefing.SayClosing();
  }
}

// The Ardennes (Operation Autumn Mist).
void opeu_brief_ardennes(Game& game, HostEvent event) {
  Briefing    briefing{ game, event };
  auto const& data{ Data(game) };
  if (data.current_side == 0) {
    briefing.Activate(0);
    briefing.Commander();
    briefing.Say(data.brief_ardennes_axis_1);
    briefing.Say(data.brief_ardennes_axis_2);
    briefing.Say(data.brief_ardennes_axis_3);
    briefing.Say(data.brief_ardennes_axis_4);
    briefing.Say(data.brief_ardennes_axis_5);
    briefing.Staff();
    briefing.Say(data.brief_ardennes_axis_6);
    briefing.Say(data.brief_ardennes_axis_7);
    briefing.HookOn();
    briefing.Mark(0x05, 0x11);
    briefing.Say(data.brief_ardennes_axis_8);
    briefing.HookOff();
    briefing.Say(data.brief_ardennes_axis_9);
    briefing.Format(data.brief_ardennes_axis_10);
    briefing.SayClosing();
  } else if (data.current_side == 1) {
    briefing.Activate(1);
    briefing.Commander();
    briefing.Say(data.brief_ardennes_allies_1);
    briefing.Say(data.brief_ardennes_allies_2);
    briefing.Say(data.brief_ardennes_allies_3);
    briefing.Say(data.brief_ardennes_allies_4);
    briefing.Staff();
    briefing.Say(data.brief_ardennes_allies_5);
    briefing.HookOn();
    briefing.Mark(0x05, 0x11);
    briefing.Say(data.brief_ardennes_allies_6);
    briefing.HookOff();
    briefing.Say(data.brief_ardennes_allies_7);
    briefing.Say(data.brief_ardennes_allies_8);
    briefing.Say(data.brief_ardennes_allies_9);
  }
}

// Berlin.
void opeu_brief_berlin(Game& game, HostEvent event) {
  Briefing    briefing{ game, event };
  auto const& data{ Data(game) };
  if (data.current_side == 0) {
    briefing.Activate(0);
    briefing.Commander();
    briefing.Say(data.brief_berlin_axis_1);
    briefing.Say(data.brief_berlin_axis_2);
    briefing.Say(data.brief_berlin_axis_3);
    briefing.Say(data.brief_berlin_axis_4);
    briefing.Say(data.brief_berlin_axis_5);
    briefing.Staff();
    briefing.Say(data.brief_berlin_axis_6);
    briefing.HookOn();
    briefing.Mark(0x19, 0x15);
    briefing.Say(data.brief_berlin_axis_7);
    briefing.Mark(0x03, 0x0a);
    briefing.Say(data.brief_berlin_axis_8);
    briefing.HookOff();
    briefing.Say(data.brief_berlin_axis_9);
    briefing.Format(data.brief_berlin_axis_10);
    briefing.SayClosing();
    briefing.Say(data.brief_berlin_axis_11);
  } else if (data.current_side == 1) {
    briefing.Activate(1);
    briefing.Commander();
    briefing.Say(data.brief_berlin_allies_1);
    briefing.Say(data.brief_berlin_allies_2);
    briefing.Say(data.brief_berlin_allies_3);
    briefing.Say(data.brief_berlin_allies_4);
    briefing.Say(data.brief_berlin_allies_5);
    briefing.Staff();
    briefing.Format(data.brief_berlin_allies_6);
    briefing.SayClosing();
    briefing.Say(data.brief_berlin_allies_7);
    briefing.HookOn();
    briefing.Mark(0x19, 0x15);
    briefing.Say(data.brief_berlin_allies_8);
    briefing.Mark(0x03, 0x0a);
    briefing.Say(data.brief_berlin_allies_9);
    briefing.HookOff();
    briefing.Say(data.brief_berlin_allies_10);
    briefing.Say(data.brief_berlin_allies_11);
  }
}

// The services in the native build.
uint16_t svc_scenbrf(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case ACTIVATE:        opeu_daily_activate(game, (uint8_t)Arg(args, count, 0)); break;
  case CONTROL_MESSAGE: opeu_control_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case OVERVIEW_PIXEL:  opeu_overview_pixel(game, Arg(args, count, 0), Arg(args, count, 1)); break;
  case INVERT:          r = (uint16_t)opeu_ui_highlight(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  case FORMAT:          opeu_request_format(game, { args, count }); break;
  default:              port_unhandled("scenbrf", id, count);
  }
  return r;
}

auto port_routine_opeu_brief_africa(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_brief_africa(game, svc_scenbrf);
  return 0;
}

auto port_routine_opeu_brief_ardennes(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_brief_ardennes(game, svc_scenbrf);
  return 0;
}

auto port_routine_opeu_brief_berlin(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_brief_berlin(game, svc_scenbrf);
  return 0;
}

auto port_routine_opeu_brief_blink(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_brief_blink(game, svc_scenbrf);
  return 0;
}

auto port_routine_opeu_brief_france(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_brief_france(game, svc_scenbrf);
  return 0;
}

auto port_routine_opeu_brief_kursk(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_brief_kursk(game, svc_scenbrf);
  return 0;
}

auto port_routine_opeu_brief_normandy(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_brief_normandy(game, svc_scenbrf);
  return 0;
}
