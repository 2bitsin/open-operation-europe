// The conversion team's credits, shown after the division panel when the player answers its key prompt with '!'.
#include "core/credits.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "glue.hpp"
#include "io/mouse.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The services the credits call through their event.
enum Service : std::uint16_t {
  INPUT_FLUSH = 0, // ()
  INPUT_A     = 1, // () -> nonzero when the mouse or a key answered
  GETCH       = 2, // () -> the key
  FILL_COLOR  = 3, // (color)
  POSITION    = 4, // (x, y)
  PRINT       = 5, // (text)
  INPUT_B     = 6, // () -> nonzero when a key or button was pressed
};

constexpr std::uint16_t CREDITS_TITLE{ offsetof(DataSegment, credits_title) };
constexpr std::uint16_t CREDITS_MAIN{ offsetof(DataSegment, credits_main) };
constexpr std::uint16_t CREDITS_SUPPORT{ offsetof(DataSegment, credits_support) };
constexpr std::uint16_t CREDITS_TRANSLATION{ offsetof(DataSegment, credits_translation) };
constexpr std::uint16_t CREDITS_X{ 17 };
} // namespace

// Waits for the panel's key; on '!' clears the screen to color 1, flags a restart of the display and shows the
// credits until a key or button is pressed. With the mouse input mode on, the mouse answers the prompt instead.
void opeu_division_credits(uint8_t* ds, HostEvent event) {
  auto& data{ Data(ds) };
  Call(ds, event, INPUT_FLUSH);
  while (data.input_mode)
    if (Call(ds, event, INPUT_A)) return;
  if (static_cast<char>(Call(ds, event, GETCH)) == '!') {
    Call(ds, event, FILL_COLOR, 1);
    data.restart_flag = 1;
    Call(ds, event, POSITION, CREDITS_X, 100);
    Call(ds, event, PRINT, CREDITS_TITLE);
    Call(ds, event, POSITION, CREDITS_X, 150);
    Call(ds, event, PRINT, CREDITS_MAIN);
    Call(ds, event, POSITION, CREDITS_X, 170);
    Call(ds, event, PRINT, CREDITS_SUPPORT);
    Call(ds, event, POSITION, CREDITS_X, 190);
    Call(ds, event, PRINT, CREDITS_TRANSLATION);
    while (!Call(ds, event, INPUT_B)) {
    }
  }
}

uint16_t svc_credits(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t result{ 0 };
  switch (id) {
  case INPUT_FLUSH: opeu_input_flush(ds, (TextHost*)port_textout_host()); break;
  case INPUT_A:     result = static_cast<uint16_t>(opeu_turn_input_a(ds, svc_mouse)); break;
  case GETCH:       result = static_cast<uint16_t>(port_getch()); break;
  case FILL_COLOR:  opeu_backdrop_fill_color(ds, (const UiHost*)port_uiprim_host(), static_cast<uint8_t>(Arg(args, count, 0))); break;
  case POSITION:    opeu_army_position(ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case PRINT:       opeu_cursor_print(ds, { args, count }); break;
  case INPUT_B:     result = opeu_turn_input_b(); break;
  default:          port_unhandled("credits", id, count);
  }
  return result;
}
