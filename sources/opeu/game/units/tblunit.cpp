// The row-draw callbacks of the regiment tables: position, order target and order with the resource percentages, and
// the regiment orders table.
#include "units/tblunit.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/table-rows.hpp"
#include "core/msglook.hpp"
#include "core/recindex.hpp"
#include "core/strings.hpp"
#include "core/tblgroup.hpp"
#include "core/textout.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/labels.hpp"
#include "units/orders.hpp"
#include "units/percent.hpp"
#include "units/table.hpp"
#include "units/unitname.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>

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
void opeu_tbl_draw_units(Game& game, DrawRequest const& request) {
  auto const [column, first, last_column, last, flags]{ request };
  auto&               data{ Data(game) };
  ServiceLocals const buffers{ TABLE_SCRATCH };
  buffers.Clear(game);
  auto const  scratch{ buffers.At(0) };
  auto const  name{ static_cast<std::uint16_t>(scratch + NAME_TEXT) };
  auto const  text{ static_cast<std::uint16_t>(scratch + ROW_BUFFER) };
  auto const  second{ static_cast<std::uint16_t>(scratch + LOCATION_TEXT) };
  auto const& window{ ActiveWindow(data) };
  auto const  layout{ TableColumnLayout(game, column, last_column) };
  if (flags & DRAW_HEADING) DrawHeading(game, window, layout, text, Near(data, data.tbl_unit_header));
  for (auto const entry : DrawnRows(first, last)) {
    auto const  unit{ data.row_units[entry] };
    auto const& regiment{ Record<Unit>(data, unit) };
    auto const  side{ opeu_unit_side(unit) };
    auto const  own{ side == data.current_side };
    auto const  ready{ (regiment.flags & Bits(UnitFlag::READY)) != 0 };
    auto const  spotted{ (regiment.flags & Bits(UnitFlag::SPOTTED)) != 0 };
    if (flags & DRAW_NAMES) {
      StartRowName(game, window, entry, NAME_INDENT, side);
      if (ready) {
        if (own && (regiment.flags & Bits(UnitFlag::SELECTED))) opeu_ui_text(game, Near(data, data.tbl_unit_mark));
        opeu_cursor_print(game, std::array<std::uint16_t, 2>{ Near(data, data.tbl_unit_label), opeu_unit_name(game, unit, name) });
      } else
        opeu_ui_text(game, Near(data, data.tbl_unit_empty));
    }
    if (!ready || (own && !opeu_unit_selection_eligible(game, unit)))
      opeu_string_copy(game, text, Near(data, data.tbl_unit_blank));
    else if ((own && opeu_unit_selection_eligible(game, unit)) || (!own && spotted)) {
      auto const destination{ UnitOrderDestination(game, unit) };
      auto const percentages{ UnitResourcePercentages(data, unit) };
      auto const target{ destination.valid ? opeu_location_name(game, destination.cell->x, destination.cell->y, name) : opeu_location_name(game, regiment.x, regiment.y, second) };
      auto const here{ opeu_location_name(game, regiment.x, regiment.y, second) };
      auto const order{ destination.valid ? data.order_names[regiment.orders[0].kind + 3] : Near(data, data.tbl_unit_wait) };
      opeu_request_format(game, std::array<std::uint16_t, 8>{ text, Near(data, data.tbl_unit_row), here, target, order, percentages.fuel, percentages.ammunition, percentages.food });
    } else if (!own && !spotted)
      opeu_request_format(game, std::array<std::uint16_t, 3>{ text, Near(data, data.tbl_unit_hidden), opeu_location_name(game, regiment.x, regiment.y, name) });
    DrawRowText(game, window, layout, entry, text);
  }
  opeu_text_color(game, TABLE_TEXT_COLOR);
}

// The regiment orders table: the regiment's name and a fixed request text.
void opeu_tbl_draw_orders(Game& game, DrawRequest const& request) {
  auto const [column, first, last_column, last, flags]{ request };
  auto&               data{ Data(game) };
  ServiceLocals const buffers{ TABLE_SCRATCH };
  buffers.Clear(game);
  auto const  scratch{ buffers.At(0) };
  auto const  name{ static_cast<std::uint16_t>(scratch + NAME_TEXT) };
  auto const  text{ static_cast<std::uint16_t>(scratch + ROW_BUFFER) };
  auto const& window{ ActiveWindow(data) };
  auto const  layout{ TableColumnLayout(game, column, last_column) };
  if (flags & DRAW_HEADING) DrawHeading(game, window, layout, text, Near(data, data.tbl_order_header));
  for (auto const entry : DrawnRows(first, last)) {
    auto const unit{ data.row_units[entry] };
    auto const side{ opeu_unit_side(unit) };
    if (flags & DRAW_NAMES) {
      StartRowName(game, window, entry, NAME_INDENT, side);
      opeu_cursor_print(game, std::array<std::uint16_t, 2>{ Near(data, data.tbl_order_label), opeu_unit_name(game, unit, name) });
    }
    opeu_string_copy(game, text, opeu_message_lookup(game, static_cast<int16_t>(ORDERS_ROW_TEXT)));
    DrawRowText(game, window, layout, entry, text);
  }
  opeu_text_color(game, TABLE_TEXT_COLOR);
}
