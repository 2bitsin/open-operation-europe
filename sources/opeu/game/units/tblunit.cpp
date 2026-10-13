// The row-draw callbacks of the regiment tables: position, order target and order with the resource percentages, and
// the regiment orders table.
#include "units/tblunit.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/table-rows.hpp"
#include "core/recindex.hpp"
#include "core/strings.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/labels.hpp"
#include "units/orders.hpp"
#include "units/percent.hpp"
#include "units/table.hpp"
#include "units/unitname.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/rqarmtbl.hpp"

using namespace opeu::game;

namespace {
// The buffers of the regiment table's frame, past the row text: a name and a second location name.
constexpr std::uint16_t NAME_TEXT{ 4 };
constexpr std::uint16_t ROW_BUFFER{ 36 };
constexpr std::uint16_t LOCATION_TEXT{ 136 };
// The request text of a regiment's orders row.
constexpr std::uint16_t ORDERS_ROW_TEXT{ 0x7b };
// Where the row names start, in columns into the window.
constexpr std::uint16_t NAME_INDENT{ 2 };
} // namespace

// The regiment table: name, position, order target and order, and fuel, ammunition and food percentages. Own
// regiments need to be selectable; enemy regiments show the full row only when spotted, otherwise just the position.
void opeu_tbl_draw_units(uint8_t ds[65536], HostEvent event, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&               data{ Data(ds) };
  ServiceLocals const buffers{ TABLE_SCRATCH };
  buffers.Clear(ds);
  auto const  scratch{ buffers.At(0) };
  auto const  name{ static_cast<std::uint16_t>(scratch + NAME_TEXT) };
  auto const  text{ static_cast<std::uint16_t>(scratch + ROW_BUFFER) };
  auto const  second{ static_cast<std::uint16_t>(scratch + LOCATION_TEXT) };
  auto const& window{ ActiveWindow(data) };
  auto const  layout{ Columns(ds, event, column, row) };
  if (flags & DRAW_HEADING) DrawHeading(ds, event, window, layout, text, Near(data, data.tbl_unit_header));
  for (auto entry{ first }; entry <= last; ++entry) {
    auto const  unit{ data.row_units[entry] };
    auto const& regiment{ Record<Unit>(data, unit) };
    auto const  side{ opeu_unit_side(unit) };
    auto const  own{ side == data.current_side };
    auto const  ready{ (regiment.flags & Bits(UnitFlag::READY)) != 0 };
    auto const  spotted{ (regiment.flags & Bits(UnitFlag::SPOTTED)) != 0 };
    if (flags & DRAW_NAMES) {
      StartRowName(ds, event, window, entry, NAME_INDENT, side);
      if (ready) {
        if (own && (regiment.flags & Bits(UnitFlag::SELECTED))) Call(ds, event, TableService::UI_TEXT, Near(data, data.tbl_unit_mark));
        Call(ds, event, TableService::PRINT, Near(data, data.tbl_unit_label), opeu_unit_name(ds, event, unit, name));
      } else
        Call(ds, event, TableService::UI_TEXT, Near(data, data.tbl_unit_empty));
    }
    if (!ready || (own && !opeu_unit_selection_eligible(ds, unit)))
      opeu_string_copy(ds, text, Near(data, data.tbl_unit_blank));
    else if ((own && opeu_unit_selection_eligible(ds, unit)) || (!own && spotted)) {
      std::uint8_t  x{};
      std::uint8_t  y{};
      std::uint16_t ammunition{};
      std::uint16_t fuel{};
      std::uint16_t food{};
      auto const    has_destination{ opeu_unit_order_destination(ds, unit, &x, &y) };
      opeu_unit_resource_percentages(ds, unit, &ammunition, &fuel, &food);
      auto const target{ has_destination ? opeu_location_name(ds, event, x, y, name) : opeu_location_name(ds, event, regiment.x, regiment.y, second) };
      auto const here{ opeu_location_name(ds, event, regiment.x, regiment.y, second) };
      auto const order{ has_destination ? data.order_names[regiment.orders[0].kind + 3] : Near(data, data.tbl_unit_wait) };
      Call(ds, event, TableService::FORMAT, text, Near(data, data.tbl_unit_row), here, target, order, fuel, ammunition, food);
    } else if (!own && !spotted)
      Call(ds, event, TableService::FORMAT, text, Near(data, data.tbl_unit_hidden), opeu_location_name(ds, event, regiment.x, regiment.y, name));
    DrawRowText(ds, event, window, layout, entry, text);
  }
  Call(ds, event, TableService::TEXT_COLOR, TABLE_TEXT_COLOR);
}

// The regiment orders table: the regiment's name and a fixed request text.
void opeu_tbl_draw_orders(uint8_t ds[65536], HostEvent event, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&               data{ Data(ds) };
  ServiceLocals const buffers{ TABLE_SCRATCH };
  buffers.Clear(ds);
  auto const  scratch{ buffers.At(0) };
  auto const  name{ static_cast<std::uint16_t>(scratch + NAME_TEXT) };
  auto const  text{ static_cast<std::uint16_t>(scratch + ROW_BUFFER) };
  auto const& window{ ActiveWindow(data) };
  auto const  layout{ Columns(ds, event, column, row) };
  if (flags & DRAW_HEADING) DrawHeading(ds, event, window, layout, text, Near(data, data.tbl_order_header));
  for (auto entry{ first }; entry <= last; ++entry) {
    auto const unit{ data.row_units[entry] };
    auto const side{ opeu_unit_side(unit) };
    if (flags & DRAW_NAMES) {
      StartRowName(ds, event, window, entry, NAME_INDENT, side);
      Call(ds, event, TableService::PRINT, Near(data, data.tbl_order_label), opeu_unit_name(ds, event, unit, name));
    }
    opeu_string_copy(ds, text, Call(ds, event, TableService::REQUEST_TEXT, ORDERS_ROW_TEXT));
    DrawRowText(ds, event, window, layout, entry, text);
  }
  Call(ds, event, TableService::TEXT_COLOR, TABLE_TEXT_COLOR);
}

// The tables' calls are the row-draw services; unit_name and location_name format through this event as well.
uint16_t svc_tblunit(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_unit_table(ds, "tblunit", id, args, count);
}

auto port_routine_opeu_tbl_draw_orders(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return RunUnitTable(ds, svc_tblunit, opeu_tbl_draw_orders, args, count);
}

auto port_routine_opeu_tbl_draw_units(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return RunUnitTable(ds, svc_tblunit, opeu_tbl_draw_units, args, count);
}
