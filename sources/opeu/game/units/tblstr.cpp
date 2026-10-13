// The row-draw callback of the item strength table: the type name in its nation's colour, strength and damage.
#include "units/tblstr.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/table-rows.hpp"
#include "core/sideutil.hpp"
#include "core/tblgroup.hpp"
#include "core/textout.hpp"
#include "types.hpp"
#include "units/table.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>

using namespace opeu::game;

namespace {
// Where the row names start, in columns into the window.
constexpr std::uint16_t NAME_INDENT{ 2 };
} // namespace

void opeu_tbl_draw_strength(Game& game, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&               data{ Data(game) };
  ServiceLocals const buffers{ TABLE_SCRATCH };
  buffers.Clear(game);
  auto const  scratch{ buffers.At(0) };
  auto const  text{ static_cast<std::uint16_t>(scratch + ROW_TEXT) };
  auto const& window{ ActiveWindow(data) };
  auto const  layout{ TableColumnLayout(game, column, row) };
  if (flags & DRAW_HEADING) DrawHeading(game, window, layout, text, Near(data, data.tbl_str_header));
  for (auto const entry : DrawnRows(first, last)) {
    auto const& item{ Record<Item>(data, data.strength_items[entry]) };
    auto const& type{ data.element_types[item.type] };
    if (flags & DRAW_NAMES) {
      StartRowName(game, window, entry, NAME_INDENT, opeu_record_side(game, Near(data, type)));
      opeu_cursor_print(game, std::array<std::uint16_t, 2>{ Near(data, data.tbl_str_name), type.name });
    }
    opeu_request_format(game, std::array<std::uint16_t, 4>{ text, Near(data, data.tbl_str_row), item.quantity, item.extra });
    DrawRowText(game, window, layout, entry, text);
  }
  opeu_text_color(game, TABLE_TEXT_COLOR);
}
