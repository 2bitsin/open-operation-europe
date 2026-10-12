// The row-draw callback of the battalion-type table the arms request shows: the equipment types collected in
// rq_type_list, each with its nation, soft and hard attack, range, speed, gun rating and mode.
#include "types.hpp"
#include "units/rqsuptbl.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "common/table-rows.hpp"
#include "units/table.hpp"
#include "core/personfl.hpp"
#include "common/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"
#include "units/rqarmtbl.hpp"

using namespace opeu::game;

namespace {

// The colour of the current side's names.
[[nodiscard]] auto SideColour(DataSegment const& data) -> std::uint16_t {
  return data.current_side ? 5 : 6;
}
} // namespace

// Draws rows first..last of the type table's `column`, with the heading and the type names as `flags` ask. The
// nation shown is the general's (context_record1) when the type is fielded by it, else the type's first nation.
void opeu_rq_draw_types(uint8_t ds[65536], HostEvent event, uint16_t scratch,
                        uint8_t column, uint8_t first, uint8_t row,
                        uint8_t last, uint8_t flags) {
  auto&       data{ Data(ds) };
  auto const  text{ static_cast<std::uint16_t>(scratch + ROW_TEXT) };
  auto const& window{ ActiveWindow(data) };
  auto const  layout{ Columns(ds, event, scratch, column, row) };
  if (flags & DRAW_HEADING) {
    Call(ds, event, TableService::FORMAT, text,
         Near(data, data.rq_type_header));
    Call(ds, event, TableService::POSITION,
         ColumnX(data, window, layout, -0x20), window.y + 1);
    Call(ds, event, TableService::TEXT_COLOR, SideColour(data));
    Call(ds, event, TableService::PUT_RANGE, text, layout.start, layout.count);
  }
  for (auto entry{ first }; entry <= last; ++entry) {
    auto const& type{ data.element_types[data.rq_type_list[entry]] };
    auto const  y{ RowY(data, window, entry) };
    if (flags & DRAW_NAMES) {
      Call(ds, event, TableService::POSITION, window.x + 3, y);
      Call(ds, event, TableService::TEXT_COLOR, SideColour(data));
      Call(ds, event, TableService::PRINT, Near(data, data.rq_type_name),
           type.name);
    }
    auto const general{ Record<Person>(data, data.context_record1).status };
    std::uint16_t const nations{ !(type.sides & general) ? type.sides
                                                         : general };
    Call(ds, event, TableService::FORMAT, text, Near(data, data.rq_type_row),
         opeu_first_mask_name(ds, nations), type.hard_rating, type.soft_rating,
         type.range, type.speed, (unsigned)(type.usage * type.weight) / 10,
         type.mode);
    DrawRowText(ds, event, window, layout, entry, text);
  }
  Call(ds, event, TableService::TEXT_COLOR, TABLE_TEXT_COLOR);
}

// The services of the type table: the row-draw services.
uint16_t svc_rqsuptbl(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count) {
  return svc_unit_table(ds, "rqsuptbl", id, args, count);
}

auto port_routine_opeu_rq_draw_types(std::uint8_t*        ds,
                                     std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  return RunUnitTable(ds, svc_rqsuptbl, opeu_rq_draw_types, args, count);
}
