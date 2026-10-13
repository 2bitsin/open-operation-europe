// The row-draw callback of the element table of a regiment: name (marked when selected), strength, damage, morale,
// training and fatigue (experience).
#include "units/tblelem.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/table-rows.hpp"
#include "core/recindex.hpp"
#include "types.hpp"
#include "units/labels.hpp"
#include "units/table.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/rqarmtbl.hpp"

using namespace opeu::game;

namespace {
// The element name buffer of the callback's frame.
constexpr std::uint16_t NAME_TEXT{ 104 };
// Where the row names start, in columns into the window.
constexpr std::uint16_t NAME_INDENT{ 2 };
} // namespace

void opeu_tbl_draw_elements(uint8_t ds[65536], HostEvent event, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&               data{ Data(ds) };
  ServiceLocals const buffers{ TABLE_SCRATCH };
  buffers.Clear(ds);
  auto const  scratch{ buffers.At(0) };
  auto const  text{ static_cast<std::uint16_t>(scratch + ROW_TEXT) };
  auto const  name{ static_cast<std::uint16_t>(scratch + NAME_TEXT) };
  auto const& window{ ActiveWindow(data) };
  auto const  layout{ Columns(ds, event, column, row) };
  if (flags & DRAW_HEADING) DrawHeading(ds, event, window, layout, text, Near(data, data.tbl_element_header));
  for (auto entry{ first }; entry <= last; ++entry) {
    auto const  near{ data.row_elements[entry] };
    auto const& element{ Record<Element>(data, near) };
    auto const  side{ opeu_element_side(near) };
    if (flags & DRAW_NAMES) {
      StartRowName(ds, event, window, entry, NAME_INDENT, side);
      if (element.flags & Bits(ElementFlag::SELECTED)) Call(ds, event, TableService::UI_TEXT, Near(data, data.tbl_element_mark));
      Call(ds, event, TableService::PRINT, Near(data, data.tbl_element_name), opeu_element_name(ds, near, name));
    }
    Call(ds, event, TableService::FORMAT, text, Near(data, data.tbl_element_row), element.quantity, element.damage, element.morale, element.training, element.experience);
    DrawRowText(ds, event, window, layout, entry, text);
  }
  Call(ds, event, TableService::TEXT_COLOR, TABLE_TEXT_COLOR);
}

// The table's calls are the row-draw services.
uint16_t svc_tblelem(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_unit_table(ds, "tblelem", id, args, count);
}

auto port_routine_opeu_tbl_draw_elements(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return RunUnitTable(ds, svc_tblelem, opeu_tbl_draw_elements, args, count);
}
