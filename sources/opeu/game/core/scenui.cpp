// The panels of the scenario-selection screen: the title frame, framed messages, panel clears and the scrolling
// scenario description.
#include "core/scenui.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The services the panels call through their event.
enum Service : std::uint16_t {
  OUTLINE      = 0, // outline(x1, y1, x2, y2, color)
  POSITION     = 1, // position(x, y)
  PRINT        = 2, // print(format, arguments...)
  UI_TEXT      = 3, // ui_text(text)
  BOX          = 4, // box(x, y, width, height, style)
  SCROLL_MODE  = 5, // scroll_mode(mode)
  WINDOW       = 6, // window(left, right, top, bottom)
  WINDOW_RESET = 7, // window_reset()
};

constexpr std::uint16_t MESSAGE_FORMAT{ offsetof(DataSegment, scenario_message_format) };

} // namespace

// The title frame; the scenario text starts below it.
void opeu_scenario_frame(uint8_t ds[65536], HostEvent event) {
  auto& data{ Data(ds) };
  Call(ds, event, OUTLINE, 0x1e, 0x10, 0x113, 0x72, 2);
  data.scenario_text_x = 4;
  data.scenario_text_y = 0x18;
}

void opeu_scenario_message(uint8_t ds[65536], HostEvent event, uint16_t text) {
  Call(ds, event, POSITION, 4, 0x19);
  Call(ds, event, PRINT, MESSAGE_FORMAT, text);
}

void opeu_scenario_clear_panel(uint8_t ds[65536], HostEvent event) {
  Call(ds, event, BOX, 3, 8, 0x22, 0x170, 7);
}

void opeu_scenario_clear_text(uint8_t ds[65536], HostEvent event) {
  Call(ds, event, BOX, 2, 0x86, 0x22, 0xb8, 7);
  Call(ds, event, POSITION, 4, 0xe0);
}

// The scenario description, scrolled inside its window.
void opeu_scenario_description(uint8_t ds[65536], HostEvent event, uint16_t text) {
  Call(ds, event, SCROLL_MODE, 1);
  Call(ds, event, WINDOW, 4, 0x25, 0x1a, 0x164);
  Call(ds, event, POSITION, 4, 0x1e);
  Call(ds, event, UI_TEXT, text);
  Call(ds, event, WINDOW_RESET);
  Call(ds, event, SCROLL_MODE, 0);
}

// The services in the native build.
uint16_t svc_scenui(uint8_t* ds, uint16_t kind, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (kind) {
  case OUTLINE:
    opeu_ui_outline(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1),
                    Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4));
    break;
  case POSITION:
    opeu_army_position(ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1));
    break;
  case PRINT:   b1_cursor_print(ds, args, count); break;
  case UI_TEXT: opeu_ui_text(ds, (TextHost*)port_textout_host(), Arg(args, count, 0)); break;
  case BOX:
    r = (uint16_t)opeu_ui_box(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3),
                              Arg(args, count, 4));
    break;
  case SCROLL_MODE:
    r = (uint16_t)opeu_text_scroll_mode(ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0));
    break;
  case WINDOW:
    opeu_text_window(ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1),
                     (int16_t)Arg(args, count, 2), (int16_t)Arg(args, count, 3));
    break;
  case WINDOW_RESET: opeu_text_window_reset(ds, (TextHost*)port_textout_host()); break;
  default:           port_unhandled("scenui", kind, count);
  }
  return r;
}
