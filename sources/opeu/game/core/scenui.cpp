// The panels of the scenario-selection screen: the title frame, framed messages, panel clears and the scrolling
// scenario description.
#include "core/scenui.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "link/glue.hpp"
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
void opeu_scenario_frame(Game& game, HostEvent event) {
  auto& data{ Data(game) };
  Call(game, event, OUTLINE, 0x1e, 0x10, 0x113, 0x72, 2);
  data.scenario_text_x = 4;
  data.scenario_text_y = 0x18;
}

void opeu_scenario_message(Game& game, HostEvent event, uint16_t text) {
  Call(game, event, POSITION, 4, 0x19);
  Call(game, event, PRINT, MESSAGE_FORMAT, text);
}

void opeu_scenario_clear_panel(Game& game, HostEvent event) {
  Call(game, event, BOX, 3, 8, 0x22, 0x170, 7);
}

void opeu_scenario_clear_text(Game& game, HostEvent event) {
  Call(game, event, BOX, 2, 0x86, 0x22, 0xb8, 7);
  Call(game, event, POSITION, 4, 0xe0);
}

// The scenario description, scrolled inside its window.
void opeu_scenario_description(Game& game, HostEvent event, uint16_t text) {
  Call(game, event, SCROLL_MODE, 1);
  Call(game, event, WINDOW, 4, 0x25, 0x1a, 0x164);
  Call(game, event, POSITION, 4, 0x1e);
  Call(game, event, UI_TEXT, text);
  Call(game, event, WINDOW_RESET);
  Call(game, event, SCROLL_MODE, 0);
}

// The services in the native build.
uint16_t svc_scenui(Game& game, uint16_t kind, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (kind) {
  case OUTLINE:     opeu_ui_outline(game, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  case POSITION:    opeu_army_position(game, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case PRINT:       opeu_cursor_print(game, { args, count }); break;
  case UI_TEXT:     opeu_ui_text(game, (TextHost*)port_textout_host(), Arg(args, count, 0)); break;
  case BOX:         r = (uint16_t)opeu_ui_box(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  case SCROLL_MODE: r = (uint16_t)opeu_text_scroll_mode(game, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0)); break;
  case WINDOW: opeu_text_window(game, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1), (int16_t)Arg(args, count, 2), (int16_t)Arg(args, count, 3)); break;
  case WINDOW_RESET: opeu_text_window_reset(game, (TextHost*)port_textout_host()); break;
  default:           port_unhandled("scenui", kind, count);
  }
  return r;
}
