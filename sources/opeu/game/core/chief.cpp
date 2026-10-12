// The operations chief's portrait panel, and the messages, waits and number prompts shown beside it.
#include "types.hpp"
#include "core/chief.hpp"
#include "core/personfl.hpp"
#include "types/events.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "glue.hpp"
#include "io/keyboard.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

extern uint16_t opeu_string_length(uint8_t ds[65536], uint16_t text);

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
constexpr std::uint16_t RANGE_FORMAT{ offsetof(DataSegment,
                                               number_range_format) };
constexpr std::uint16_t PANEL_X{ 0x35 };
constexpr std::uint16_t PANEL_Y{ 0x130 };

auto Chief(std::uint8_t* ds) -> std::uint16_t {
  return Data(ds).context_record2;
}
} // namespace

// Draws the panel: a frame wide enough for the chief's name, the name and the portrait; leaves the text cursor in it.
uint16_t opeu_ops_chief_show(uint8_t ds[65536], HostEvent event) {
  opeu_person_lacks_side0_flag(ds, Chief(ds));
  Call(ds, event, FILL, PANEL_X, PANEL_Y, 0x18, 0x40, 0);
  Call(ds, event, FRAME, 0x2a, 0x10a,
       static_cast<std::uint8_t>(opeu_string_length(ds, Chief(ds)) + 13));
  Call(ds, event, PRINT, CHIEF_FORMAT, Chief(ds));
  Call(ds, event, PORTRAIT, 0x2b, 0x131, Chief(ds));
  Call(ds, event, POSITION, PANEL_X, PANEL_Y);
  return 1;
}

void opeu_ops_chief_text(uint8_t ds[65536], HostEvent event, uint16_t text) {
  if (opeu_ops_chief_show(ds, event))
    Call(ds, event, MESSAGE, text);
}

// Shows the text and the range "(low-high)" formatted into `buffer`, then reads a number in the range.
uint16_t opeu_number_prompt(uint8_t ds[65536], HostEvent event, uint16_t text,
                            uint16_t low, uint16_t high, uint16_t value,
                            uint16_t d, uint16_t buffer) {
  Call(ds, event, FORMAT, buffer, RANGE_FORMAT, low, high);
  Call(ds, event, MESSAGE, text);
  Call(ds, event, MESSAGE, buffer);
  return Call(ds, event, INPUT, low, high, value, d);
}

uint16_t opeu_ops_chief_number(uint8_t ds[65536], HostEvent event,
                               uint16_t text, uint16_t low, uint16_t high,
                               uint16_t value, uint16_t d, uint16_t buffer) {
  opeu_ops_chief_show(ds, event);
  return opeu_number_prompt(ds, event, text, low, high, value, d, buffer);
}

uint16_t opeu_ops_chief_wait(uint8_t ds[65536], HostEvent event,
                             uint16_t text) {
  opeu_ops_chief_show(ds, event);
  return Call(ds, event, WAIT, text);
}

uint16_t svc_chief(uint8_t* ds, uint16_t id, const uint16_t* args,
                   uint16_t count) {
  uint16_t result{ 0 };
  switch (id) {
    case FILL:
      opeu_ui_box(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2),
                  Arg(args, count, 3), Arg(args, count, 4));
      break;
    case FRAME:
      opeu_context_frame(ds, (const UiHost*)port_uiprim_host(),
                         Arg(args, count, 0), Arg(args, count, 1),
                         Arg(args, count, 2));
      break;
    case FORMAT: b2_request_format(ds, args, count); break;
    case PRINT:  b2_cursor_print(ds, args, count); break;
    case PORTRAIT:
      opeu_context_portrait(ds, (const UiHost*)port_uiprim_host(),
                            Arg(args, count, 0), Arg(args, count, 1),
                            Arg(args, count, 2));
      break;
    case POSITION:
      opeu_army_position(ds, (TextHost*)port_textout_host(),
                         (int16_t)Arg(args, count, 0),
                         (int16_t)Arg(args, count, 1));
      break;
    case MESSAGE:
      opeu_text_feed(ds, (const DialogHost*)port_dialogs_host(),
                     Arg(args, count, 0));
      break;
    case WAIT:
      result = opeu_confirm_prompt(ds, (const DialogHost*)port_dialogs_host(),
                                   Arg(args, count, 0));
      break;
    case INPUT:
      result = WithStack(PORT_FRAME, [&](std::uint16_t sp) {
        return opeu_choose_value(
            ds, svc_keyboard, Arg(args, count, 0), Arg(args, count, 1),
            (int16_t)Arg(args, count, 2), Arg(args, count, 3),
            static_cast<std::uint16_t>(sp - 2 - 16));
      });
      break;
    default: port_unhandled("chief", id, count);
  }
  return result;
}
