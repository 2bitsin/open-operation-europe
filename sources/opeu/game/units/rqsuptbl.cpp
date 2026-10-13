// The row-draw callback of the battalion-type table the arms request shows: the equipment types collected in
// rq_type_list, each with its nation, soft and hard attack, range, speed, gun rating and mode.
#include "units/rqsuptbl.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "common/table-rows.hpp"
#include "core/personfl.hpp"
#include "core/tblgroup.hpp"
#include "core/textout.hpp"
#include "types.hpp"
#include "units/table.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>

using namespace opeu::game;

namespace { } // namespace

// Draws rows first..last of the type table's `column`, with the heading and the type names as `flags` ask. The
// nation shown is the general's (context_record1) when the type is fielded by it, else the type's first nation.
void opeu_rq_draw_types(Game& game, DrawRequest const& request) {
  auto const [column, first, last_column, last, flags]{ request };
  auto&               data{ Data(game) };
  ServiceLocals const buffers{ TABLE_SCRATCH };
  buffers.Clear(game);
  auto const  scratch{ buffers.At(0) };
  auto const  text{ static_cast<std::uint16_t>(scratch + ROW_TEXT) };
  auto const& window{ ActiveWindow(data) };
  auto const  layout{ TableColumnLayout(game, column, last_column) };
  if (flags & DRAW_HEADING) {
    opeu_request_format(game, std::array<std::uint16_t, 2>{ text, Near(data, data.rq_type_header) });
    opeu_army_position(game, static_cast<int16_t>(ColumnX(data, window, layout, -0x20)), static_cast<int16_t>(window.y + 1));
    opeu_text_color(game, SideTextColor(data.current_side));
    opeu_text_put_range(game, text, static_cast<uint8_t>(layout.start), static_cast<uint8_t>(layout.count));
  }
  for (auto const entry : DrawnRows(first, last)) {
    auto const& type{ data.element_types[data.rq_type_list[entry]] };
    auto const  y{ RowY(data, window, entry) };
    if (flags & DRAW_NAMES) {
      opeu_army_position(game, static_cast<int16_t>(window.x + 3), static_cast<int16_t>(y));
      opeu_text_color(game, SideTextColor(data.current_side));
      opeu_cursor_print(game, std::array<std::uint16_t, 2>{ Near(data, data.rq_type_name), type.name });
    }
    auto const          general{ Record<Person>(data, data.context_record1).status };
    std::uint16_t const nations{ !(type.sides & general) ? type.sides : general };
    opeu_request_format(game, std::array<std::uint16_t, 9>{ text, Near(data, data.rq_type_row), opeu_first_mask_name(game, nations), type.anti_personnel, type.anti_tank, type.defense, type.speed,
                                                            static_cast<unsigned>(type.fuel_economy * type.fuel_capacity) / 10, type.gun_range });
    DrawRowText(game, window, layout, entry, text);
  }
  opeu_text_color(game, TABLE_TEXT_COLOR);
}
