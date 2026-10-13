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

#include "common/game.hpp"
#include "core/msglook.hpp"
#include "core/sideutil.hpp"
#include "core/textout.hpp"
#include "io/listui.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>

using namespace opeu::game;

namespace {
// The unit name buffer in the callback's frame buffers, after the row text.
constexpr std::uint16_t NAME_TEXT{ 0x70 };
} // namespace

// Draws rows first..last of the regiment table's `column`, with the heading and the unit names as `flags` ask.
void opeu_rq_draw_units(Game& game, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&               data{ Data(game) };
  ServiceLocals const buffers{ TABLE_SCRATCH };
  buffers.Clear(game);
  auto const  scratch{ buffers.At(0) };
  auto const  name{ static_cast<std::uint16_t>(scratch + NAME_TEXT) };
  auto const  text{ static_cast<std::uint16_t>(scratch + ROW_TEXT) };
  auto const& window{ ActiveWindow(data) };
  auto const  layout{ TableColumnLayout(game, column, row) };
  if (flags & DRAW_HEADING) DrawHeading(game, window, layout, text, Near(data, data.rq_unit_header));
  for (auto entry{ first }; entry <= last; ++entry) {
    auto const unit{ data.ai_unit_list[entry] };
    if (flags & DRAW_NAMES) {
      StartRowName(game, window, entry, 3, data.current_side);
      opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.rq_unit_name)), static_cast<std::uint16_t>(opeu_unit_name(game, unit, name)) });
    }
    opeu_string_copy(game, text, Near(data, data.rq_unit_row));
    DrawRowText(game, window, layout, entry, text);
  }
  opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), TABLE_TEXT_COLOR);
}

auto port_routine_opeu_rq_draw_units(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return RunTableRows(game, opeu_rq_draw_units, args, count);
}
