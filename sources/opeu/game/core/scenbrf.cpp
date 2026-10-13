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

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

// The buffer that receives the closing text formatted with the player's name.
constexpr std::uint16_t CLOSING_BYTES{ 100 };

// One side's briefing in progress: who speaks, and the closing-text buffer in the briefing's reserve.
class Briefing {
public:
  explicit Briefing(Game& game) : _game{ game } { }

  auto Activate(std::uint8_t side) -> void {
    opeu_daily_activate(_game, side);
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
    auto const&                        data{ Data(_game) };
    std::array<std::uint16_t, 3> const words{ _closing.At(0), Near(data, text), data.initial_person };
    opeu_request_format(_game, words);
  }

  auto HookOn() -> void {
    Data(_game).message_hook = MessageHook::BRIEFING_BLINK;
  }
  auto HookOff() -> void {
    Data(_game).message_hook = MessageHook::NONE;
  }
  auto Mark(std::uint8_t x, std::uint8_t y) -> void {
    opeu_brief_mark(_game, x, y);
  }

private:
  auto SayAt(std::uint16_t text) -> void {
    opeu_control_message(_game, static_cast<const DialogHost*>(port_dialogs_host()), text, _speaker);
  }

  Game&               _game;
  ServiceLocals const _closing{ CLOSING_BYTES };
  std::uint16_t       _speaker{};
};

} // namespace

// Marks an overview-map cell for the blinking hook.
void opeu_brief_mark(Game& game, uint8_t x, uint8_t y) {
  auto& data{ Data(game) };
  data.brief_mark_x = x;
  data.brief_mark_y = y;
  opeu_overview_pixel(game, offsetof(DataSegment, brief_mark_x), offsetof(DataSegment, brief_mark_y));
}

// France 1940.
void opeu_brief_france(Game& game) {
  Briefing    briefing{ game };
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
void opeu_brief_africa(Game& game) {
  Briefing    briefing{ game };
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
void opeu_brief_kursk(Game& game) {
  Briefing    briefing{ game };
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
void opeu_brief_normandy(Game& game) {
  Briefing    briefing{ game };
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
void opeu_brief_ardennes(Game& game) {
  Briefing    briefing{ game };
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
void opeu_brief_berlin(Game& game) {
  Briefing    briefing{ game };
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
