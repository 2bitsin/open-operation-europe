// The army heading (side name, commander, HQ) and the division heading (number, commander, portrait) of the panels.
#include "core/headings.hpp"
#include "common/events.hpp"
#include "core/sideutil.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The drawing services the headings call through their event.
enum Service : std::uint16_t {
  FILL     = 0, // (x0, y0, x1, y1, color)
  OUTLINE  = 1, // (x0, y0, x1, y1, color)
  CURSOR   = 2, // (x, y)
  TEXT     = 3, // (text)
  PRINT    = 5, // (format, words...)
  PORTRAIT = 6, // (x, y, person)
  LINE     = 7, // (x0, y0, x1, y1, color)
};

constexpr std::uint16_t ARMY_PERSON{ offsetof(DataSegment, army_person_format) };
constexpr std::uint16_t ARMY_HQ{ offsetof(DataSegment, army_hq_label) };
constexpr std::uint16_t DIVISION_LABEL{ offsetof(DataSegment, division_label_format) };
constexpr std::uint16_t DIVISION_PERSON{ offsetof(DataSegment, division_person_format) };
constexpr std::uint16_t COLOR_WHITE{ 7 };

} // namespace

void opeu_army_heading(uint8_t ds[65536], HostNotify event, SideColorEvent color_event, uint16_t person) {
  auto const& data{ Data(ds) };
  Call(ds, event, FILL, 6, 0, 73, 50, 0);
  Call(ds, event, OUTLINE, 6, 0, 73, 51, COLOR_WHITE);
  Call(ds, event, CURSOR, 1, 2);
  Call(ds, event, TEXT, data.side_names[data.current_side]);
  opeu_select_side_color(ds, color_event, data.current_side);
  Call(ds, event, CURSOR, 1, 18);
  Call(ds, event, PRINT, ARMY_PERSON, SideTextColor(data.current_side), person);
  Call(ds, event, CURSOR, 1, 34);
  Call(ds, event, PRINT, ARMY_HQ);
  Call(ds, event, LINE, 40, 51, 40, 186, COLOR_WHITE);
}

void opeu_division_heading(uint8_t ds[65536], HostNotify event, uint16_t person) {
  auto& data{ Data(ds) };
  Call(ds, event, FILL, 6, 143, 73, 256, 0);
  Call(ds, event, OUTLINE, 5, 142, 73, 257, COLOR_WHITE);
  Call(ds, event, CURSOR, 1, 225);
  Call(ds, event, PRINT, DIVISION_LABEL, SideTextColor(data.current_side), (Record<Person>(data, person).assignment & Bits(Assignment::DIVISION_MASK)) + 1);
  Call(ds, event, CURSOR, 1, 241);
  Call(ds, event, PRINT, DIVISION_PERSON, SideTextColor(data.current_side), person);
  Call(ds, event, PORTRAIT, 1, 144, person);
}

void svc_headings(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  switch (id) {
  case FILL:     opeu_ui_fill(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  case OUTLINE:  opeu_ui_outline(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  case CURSOR:   opeu_army_position(ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case TEXT:     opeu_ui_text(ds, (TextHost*)port_textout_host(), Arg(args, count, 0)); break;
  case PRINT:    opeu_cursor_print(ds, { args, count }); break;
  case PORTRAIT: opeu_context_portrait(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2)); break;
  case LINE:     opeu_ui_line(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  default:       port_unhandled("headings", id, count);
  }
}
