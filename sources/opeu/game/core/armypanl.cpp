// The division panel's regiments: for each ready regiment of a division a battle frame, its heading and, when it can
// act, a slot grid with an icon, name and count for each of its elements.
#include "core/armypanl.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "glue.hpp"
#include "map/support.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/labels.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The services the panel calls through its event, with their words in order.
enum Service : std::uint16_t {
  FILL     = 0, // (x1, y1, x2, y2, colour)
  OUTLINE  = 1, // (x1, y1, x2, y2, colour)
  LINE     = 2, // (x1, y1, x2, y2, colour)
  POSITION = 3, // (x, y)
  PRINT    = 4, // (format, words...)
  DRAW     = 5, // (x, y, width, height, mode, segment, offset) a far image
  NAME     = 6, // (element, buffer) -> the buffer with the element's name
  ELIGIBLE = 7, // (unit) -> nonzero when the regiment can act
};

constexpr std::uint16_t NAME_FORMAT{ offsetof(DataSegment, panel_name_format) };
constexpr std::uint16_t COUNT_FORMAT{ offsetof(DataSegment, panel_count_format) };
constexpr std::uint16_t REGIMENT_FORMAT{ offsetof(DataSegment, panel_regiment_format) };
constexpr std::uint16_t BATTLE_TEXT{ offsetof(DataSegment, panel_battle) };
constexpr std::uint16_t STRING_FORMAT{ offsetof(DataSegment, panel_string_format) };
constexpr std::uint16_t ICONS{ 0x660 }; // eleven 64-byte element icons in the frame segment
constexpr std::uint16_t ICON_BYTES{ 64 };
constexpr std::uint16_t WHITE{ 7 };
constexpr std::uint16_t NAME_BUFFER{ 0x2a }; // the element name buffer the name service writes

} // namespace

// The slot grid: two columns of `count` rows below y.
void opeu_panel_slots(uint8_t ds[65536], HostEvent event, int y, uint8_t count) {
  for (int row{ 0 }; row < count; ++row)
    for (int column{ 0 }; column < 2; ++column) {
      auto const x{ 232 * column };
      auto const top{ 17 * row + y };
      Call(ds, event, FILL, x + 175, top - 1, x + 400, top + 15, 0);
      Call(ds, event, OUTLINE, x + 175, top - 1, x + 400, top + 16, WHITE);
      Call(ds, event, LINE, x + 207, top - 1, x + 207, top + 16, WHITE);
      Call(ds, event, LINE, x + 375, top - 1, x + 375, top + 16, WHITE);
    }
}

// The far address (segment in the high word) of the icon of an element's type, chosen by its category and flags.
uint32_t opeu_element_icon(const uint8_t ds[65536], uint16_t element) {
  auto const&  data{ Data(ds) };
  auto const&  type{ ElementTypeOf(data, element) };
  std::uint8_t icon{ 0 };
  switch (type.category) {
  case 1:  icon = 1; break;
  case 2:  icon = 2; break;
  case 3:  icon = (type.flags & Bits(TypeFlag::LIGHT)) ? 3 : (type.flags & Bits(TypeFlag::SELF_PROPELLED)) ? 5 : 4; break;
  case 4:  icon = (type.flags & Bits(TypeFlag::HEAVY)) ? 7 : 6; break;
  case 5:  icon = 8; break;
  case 6:  icon = 9; break;
  case 7:  icon = 10; break;
  default: icon = 0; break;
  }
  return static_cast<std::uint32_t>(FRAME_SEGMENT) << 16 | static_cast<std::uint16_t>(ICONS + ICON_BYTES * icon);
}

// Icon, name and count of each element of the unit with a nonzero count, two per row from y.
void opeu_panel_elements(uint8_t ds[65536], HostEvent event, uint16_t unit, int y) {
  ServiceLocals const locals{ NAME_BUFFER };
  auto&               data{ Data(ds) };
  auto const          buffer{ locals.At(0) };
  int                 shown{ 0 };
  // The walk starts at the unit's first element and stops after the element whose next link is 0.
  for (auto near{ Record<Unit>(data, unit).first };;) {
    auto const& element{ Record<Element>(data, near) };
    if (element.quantity) {
      auto const icon{ opeu_element_icon(ds, near) };
      data.gfx_source[0] = static_cast<std::uint16_t>(icon);
      data.gfx_source[1] = static_cast<std::uint16_t>(icon >> 16);
      auto const x{ 29 * (shown % 2) };
      auto const row_y{ 17 * (shown / 2) + y };
      Call(ds, event, DRAW, x + 22, row_y, 4, 16, data.current_side ? 0x305 : 0x306, data.gfx_source[1], data.gfx_source[0]);
      Call(ds, event, POSITION, x + 27, row_y);
      Call(ds, event, PRINT, NAME_FORMAT, Call(ds, event, NAME, near, buffer));
      Call(ds, event, POSITION, x + 47, row_y);
      Call(ds, event, PRINT, COUNT_FORMAT, SideTextColor(data.current_side), element.quantity);
      ++shown;
    }
    if (!element.next) break;
    near = element.next;
  }
}

// The battle frame of one regiment at y, joined to the division's line.
void opeu_panel_battle_frame(uint8_t ds[65536], HostEvent event, int y) {
  Call(ds, event, LINE, 73, 201, 84, 201, WHITE);
  Call(ds, event, FILL, 94, y, 161, y + 34, 0);
  Call(ds, event, OUTLINE, 94, y, 161, y + 35, WHITE);
  Call(ds, event, LINE, 84, y + 16, 94, y + 16, WHITE);
  Call(ds, event, LINE, 84, y + 16, 84, 200, WHITE);
}

// The eight regiments of a division from `units` (the view menu's division entry): regiments 1-4 are the battle
// regiments, 5-8 carry their own names.
void opeu_division_regiments(uint8_t ds[65536], HostEvent event, uint16_t units, uint16_t person) {
  auto& data{ Data(ds) };
  for (int i{ 0 }; i < 8; ++i, units = static_cast<std::uint16_t>(units + sizeof(Unit))) {
    if (!(Record<Unit>(data, units).flags & Bits(UnitFlag::READY))) continue;
    bool const battle{ i < 4 };
    int        y{ battle ? 59 * i + 8 : 42 * i + 68 };
    opeu_panel_battle_frame(ds, event, y);
    y += battle ? 10 : 2;
    Call(ds, event, POSITION, 12, battle ? y - 7 : y - 1);
    Call(ds, event, PRINT, REGIMENT_FORMAT, SideTextColor(data.current_side), (Record<Person>(data, person).assignment & Bits(Assignment::DIVISION_MASK)) + 1, i + 1);
    if (battle) {
      Call(ds, event, POSITION, 12, y + 9);
      Call(ds, event, PRINT, BATTLE_TEXT);
      y -= 16;
      if (Call(ds, event, ELIGIBLE, units)) opeu_panel_slots(ds, event, y, 3);
    } else {
      Call(ds, event, POSITION, 12, y + 15);
      Call(ds, event, PRINT, STRING_FORMAT, data.panel_regiment_names[i - 4]);
      if (Call(ds, event, ELIGIBLE, units)) opeu_panel_slots(ds, event, y, 2);
    }
    if (Call(ds, event, ELIGIBLE, units)) opeu_panel_elements(ds, event, units, y);
  }
}

uint16_t svc_armypanl(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t result{ 0 };
  switch (id) {
  case FILL:     opeu_ui_fill(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  case OUTLINE:  opeu_ui_outline(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  case LINE:     opeu_ui_line(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  case POSITION: opeu_army_position(ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case PRINT:    opeu_cursor_print(ds, { args, count }); break;
  case DRAW:     opeu_draw_far(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), port_far(Arg(args, count, 5), Arg(args, count, 6))); break;
  case NAME:     result = opeu_element_name(ds, Arg(args, count, 0), Arg(args, count, 1)); break;
  case ELIGIBLE: result = opeu_unit_selection_eligible(ds, Arg(args, count, 0)); break;
  default:       port_unhandled("armypanl", id, count);
  }
  return result;
}
