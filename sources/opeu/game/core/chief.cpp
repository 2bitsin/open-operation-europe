// The operations chief's portrait panel, and the messages, waits and number prompts shown beside it.
#include "core/chief.hpp"
#include "common/events.hpp"
#include "core/personfl.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "io/keyboard.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

extern uint16_t opeu_string_length(Game& game, uint16_t text);

using namespace opeu::game;

namespace {

// The services the panel calls through its event.
enum Service : std::uint16_t {
  FILL     = 0, // (x, y, width, height, color)
  FRAME    = 1, // (x, y, width)
  FORMAT   = 2, // (buffer, format, low, high)
  PRINT    = 3, // (format, person)
  PORTRAIT = 4, // (x, y, person)
  POSITION = 5, // (x, y)
  MESSAGE  = 6, // (text)
  WAIT     = 7, // (text) -> the key
  INPUT    = 8, // (low, high, value, items) -> the number chosen
};

constexpr std::uint16_t CHIEF_FORMAT{ offsetof(DataSegment, ops_chief_format) };
constexpr std::uint16_t RANGE_FORMAT{ offsetof(DataSegment, number_range_format) };
constexpr std::uint16_t RANGE_TEXT_BYTES{ 0x1e }; // the "(low-high)" text
constexpr std::uint16_t PANEL_X{ 0x35 };
constexpr std::uint16_t PANEL_Y{ 0x130 };

auto Chief(Game& game) -> std::uint16_t {
  return Data(game).context_record2;
}
} // namespace

// Draws the panel: a frame wide enough for the chief's name, the name and the portrait; leaves the text cursor in it.
uint16_t opeu_ops_chief_show(Game& game, HostEvent event) {
  opeu_person_lacks_side0_flag(game, Chief(game));
  Call(game, event, FILL, PANEL_X, PANEL_Y, 0x18, 0x40, 0);
  Call(game, event, FRAME, 0x2a, 0x10a, static_cast<std::uint8_t>(opeu_string_length(game, Chief(game)) + 13));
  Call(game, event, PRINT, CHIEF_FORMAT, Chief(game));
  Call(game, event, PORTRAIT, 0x2b, 0x131, Chief(game));
  Call(game, event, POSITION, PANEL_X, PANEL_Y);
  return 1;
}

void opeu_ops_chief_text(Game& game, HostEvent event, uint16_t text) {
  if (opeu_ops_chief_show(game, event)) Call(game, event, MESSAGE, text);
}

// Shows the text and the range "(low-high)" formatted into `buffer`, then reads a number in the range.
uint16_t opeu_number_prompt(Game& game, HostEvent event, uint16_t text, uint16_t low, uint16_t high, uint16_t value, uint16_t d, uint16_t buffer) {
  Call(game, event, FORMAT, buffer, RANGE_FORMAT, low, high);
  Call(game, event, MESSAGE, text);
  Call(game, event, MESSAGE, buffer);
  return Call(game, event, INPUT, low, high, value, d);
}

// The operations chief asks for a number in low..high; the range text is formatted in the routine's reserve.
uint16_t opeu_ops_chief_number(Game& game, HostEvent event, uint16_t text, uint16_t low, uint16_t high, uint16_t value, uint16_t d) {
  ServiceLocals const range{ RANGE_TEXT_BYTES };
  opeu_ops_chief_show(game, event);
  return opeu_number_prompt(game, event, text, low, high, value, d, range.At(0));
}

uint16_t opeu_ops_chief_wait(Game& game, HostEvent event, uint16_t text) {
  opeu_ops_chief_show(game, event);
  return Call(game, event, WAIT, text);
}

uint16_t svc_chief(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t result{ 0 };
  switch (id) {
  case FILL:     opeu_ui_box(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  case FRAME:    opeu_context_frame(game, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2)); break;
  case FORMAT:   opeu_request_format(game, { args, count }); break;
  case PRINT:    opeu_cursor_print(game, { args, count }); break;
  case PORTRAIT: opeu_context_portrait(game, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2)); break;
  case POSITION: opeu_army_position(game, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case MESSAGE:  opeu_text_feed(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case WAIT:     result = opeu_confirm_prompt(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case INPUT:    result = opeu_choose_value(game, svc_keyboard, Arg(args, count, 0), Arg(args, count, 1), (int16_t)Arg(args, count, 2), Arg(args, count, 3)); break;
  default:       port_unhandled("chief", id, count);
  }
  return result;
}
