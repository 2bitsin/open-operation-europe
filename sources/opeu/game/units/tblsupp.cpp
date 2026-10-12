// The row-draw callback of the city supply table: fuel, ammunition and food stocks with their percentages, the
// garrison strength and its limit. Own cities show their stocks when eligible, enemy cities only when reconnoitred.
#include "units/tblsupp.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/table-rows.hpp"
#include "core/strings.hpp"
#include "map/objeleg.hpp"
#include "map/query.hpp"
#include "types.hpp"
#include "units/objpct.hpp"
#include "units/table.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/rqarmtbl.hpp"

using namespace opeu::game;

namespace {
// Where the row names start, in columns into the window.
constexpr std::uint16_t NAME_INDENT{ 3 };
} // namespace

void opeu_tbl_draw_supplies(uint8_t ds[65536], HostEvent event, uint16_t scratch, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&       data{ Data(ds) };
  auto const  text{ static_cast<std::uint16_t>(scratch + ROW_TEXT) };
  auto const& window{ ActiveWindow(data) };
  auto const  layout{ Columns(ds, event, scratch, column, row) };
  if (flags & DRAW_HEADING) DrawHeading(ds, event, window, layout, text, Near(data, data.tbl_supply_header));
  for (auto entry{ first }; entry <= last; ++entry) {
    auto const  near{ data.tbl_objective_list[entry] };
    auto const& city{ Record<Objective>(data, near) };
    auto const  side{ opeu_objective_side(ds, near) };
    if (flags & DRAW_NAMES) {
      StartRowName(ds, event, window, entry, NAME_INDENT, side);
      Call(ds, event, TableService::PRINT, Near(data, data.tbl_supply_name), near);
    }
    if ((side == data.current_side && opeu_objective_eligible(ds, near)) || (side != data.current_side && (city.flags & Bits(ObjectiveFlag::MARKED)))) {
      std::uint16_t ammunition{};
      std::uint16_t fuel{};
      std::uint16_t food{};
      opeu_objective_resource_percentages(ds, near, &ammunition, &fuel, &food);
      Call(ds, event, TableService::FORMAT, text, Near(data, data.tbl_supply_row), city.resources[1], fuel, city.resources[0], ammunition, city.resources[2], food, city.strength, city.strength_limit);
    } else
      opeu_string_copy(ds, text, Near(data, data.tbl_supply_blank));
    DrawRowText(ds, event, window, layout, entry, text);
  }
  Call(ds, event, TableService::TEXT_COLOR, TABLE_TEXT_COLOR);
}

// The table's calls are the row-draw services.
uint16_t svc_tblsupp(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_unit_table(ds, "tblsupp", id, args, count);
}

auto port_routine_opeu_tbl_draw_supplies(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return RunUnitTable(ds, svc_tblsupp, opeu_tbl_draw_supplies, args, count);
}
