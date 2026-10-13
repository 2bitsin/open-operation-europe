// The row-draw callback of the regiment table the regiment request shows: the free regiments collected in
// ai_unit_list, each a unit name and a blank strength row. Also the services every list table's row-draw callback
// calls.
#include "units/rqarmtbl.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/table-rows.hpp"
#include "core/strings.hpp"
#include "core/tblgroup.hpp"
#include "types.hpp"
#include "units/table.hpp"
#include "units/unitname.hpp"

#include "core/msglook.hpp"
#include "core/sideutil.hpp"
#include "core/textout.hpp"
#include "glue.hpp"
#include "io/listui.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// The unit name buffer in the callback's frame buffers, after the row text.
constexpr std::uint16_t NAME_TEXT{ 0x70 };
} // namespace

// Draws rows first..last of the regiment table's `column`, with the heading and the unit names as `flags` ask.
void opeu_rq_draw_units(uint8_t ds[65536], HostEvent event, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&               data{ Data(ds) };
  ServiceLocals const buffers{ TABLE_SCRATCH };
  buffers.Clear(ds);
  auto const  scratch{ buffers.At(0) };
  auto const  name{ static_cast<std::uint16_t>(scratch + NAME_TEXT) };
  auto const  text{ static_cast<std::uint16_t>(scratch + ROW_TEXT) };
  auto const& window{ ActiveWindow(data) };
  auto const  layout{ Columns(ds, event, column, row) };
  if (flags & DRAW_HEADING) DrawHeading(ds, event, window, layout, text, Near(data, data.rq_unit_header));
  for (auto entry{ first }; entry <= last; ++entry) {
    auto const unit{ data.ai_unit_list[entry] };
    if (flags & DRAW_NAMES) {
      StartRowName(ds, event, window, entry, 3, data.current_side);
      Call(ds, event, TableService::PRINT, Near(data, data.rq_unit_name), opeu_unit_name(ds, event, unit, name));
    }
    opeu_string_copy(ds, text, Near(data, data.rq_unit_row));
    DrawRowText(ds, event, window, layout, entry, text);
  }
  Call(ds, event, TableService::TEXT_COLOR, TABLE_TEXT_COLOR);
}

// The services of the regiment table: the row-draw services.
uint16_t svc_rqarmtbl(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_table(ds, "rqarmtbl", id, args, count);
}

auto port_routine_opeu_rq_draw_units(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return RunTableRows(ds, svc_rqarmtbl, opeu_rq_draw_units, args, count);
}
