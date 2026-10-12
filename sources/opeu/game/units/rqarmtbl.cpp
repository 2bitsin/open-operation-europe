// The row-draw callback of the regiment table the regiment request shows: the free regiments collected in
// ai_unit_list, each a unit name and a blank strength row. Also the services every list table's row-draw callback
// calls.
#include "units/rqarmtbl.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/table-rows.hpp"
#include "core/strings.hpp"
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
void opeu_rq_draw_units(uint8_t ds[65536], HostEvent event, uint16_t /*scratch*/, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&               data{ Data(ds) };
  ServiceLocals const buffers{ TABLE_SCRATCH };
  auto const          scratch{ buffers.At(0) };
  auto const          name{ static_cast<std::uint16_t>(scratch + NAME_TEXT) };
  auto const          text{ static_cast<std::uint16_t>(scratch + ROW_TEXT) };
  auto const&         window{ ActiveWindow(data) };
  auto const          layout{ Columns(ds, event, scratch, column, row) };
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
  return svc_unit_table(ds, "rqarmtbl", id, args, count);
}

uint16_t svc_unit_table(uint8_t* ds, const char* module, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case TableService::COLUMNS:      opeu_table_columns(ds, svc_listui, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  case TableService::POSITION:     opeu_army_position(ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case TableService::FORMAT:       opeu_request_format(ds, { args, count }); break;
  case TableService::TEXT_COLOR:   opeu_text_color(ds, (TextHost*)port_textout_host(), Arg(args, count, 0)); break;
  case TableService::SIDE_COLOR:   opeu_select_side_color(ds, svc_sideutil, (uint8_t)Arg(args, count, 0)); break;
  case TableService::UI_TEXT:      opeu_ui_text(ds, (TextHost*)port_textout_host(), Arg(args, count, 0)); break;
  case TableService::PRINT:        opeu_cursor_print(ds, { args, count }); break;
  case TableService::PUT_RANGE:    opeu_text_put_range(ds, (TextHost*)port_textout_host(), Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case TableService::REQUEST_TEXT: r = opeu_message_lookup(ds, svc_msglook, (int16_t)Arg(args, count, 0)); break;
  default:                         port_unhandled(module, id, count);
  }
  return r;
}

auto port_routine_opeu_rq_draw_units(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return RunUnitTable(ds, svc_rqarmtbl, opeu_rq_draw_units, args, count);
}
