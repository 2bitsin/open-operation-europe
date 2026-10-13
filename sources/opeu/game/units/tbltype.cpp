// The row-draw callback of the equipment type table: nation, hard and soft ratings, range, speed, usage times weight
// / 10 and mode. The nation column names the current general's first nation when the type is fielded by one of the
// general's nations, otherwise the type's own first nation.
#include "units/tbltype.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/table-rows.hpp"
#include "core/personfl.hpp"
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
constexpr std::uint16_t NAME_INDENT{ 3 };
} // namespace

void opeu_tbl_draw_types(Game& game, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&               data{ Data(game) };
  ServiceLocals const buffers{ TABLE_SCRATCH };
  buffers.Clear(game);
  auto const  scratch{ buffers.At(0) };
  auto const  text{ static_cast<std::uint16_t>(scratch + ROW_TEXT) };
  auto const& window{ ActiveWindow(data) };
  auto const  layout{ TableColumnLayout(game, column, row) };
  if (flags & DRAW_HEADING) DrawHeading(game, window, layout, text, Near(data, data.tbl_type_header));
  for (auto const entry : DrawnRows(first, last)) {
    auto const  near{ data.tbl_type_list[entry] };
    auto const& type{ Record<ElementType>(data, near) };
    if (flags & DRAW_NAMES) {
      StartRowName(game, window, entry, NAME_INDENT, opeu_record_side(game, near));
      opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.tbl_type_name)), static_cast<std::uint16_t>(type.name) });
    }
    auto const    general{ Record<Person>(data, data.context_record1).status };
    std::uint16_t nations{ !(type.sides & general) ? type.sides : general };
    opeu_request_format(game,
                        std::array<std::uint16_t, 9>{ static_cast<std::uint16_t>(text), static_cast<std::uint16_t>(Near(data, data.tbl_type_row)),
                                                      static_cast<std::uint16_t>(opeu_first_mask_name(game, static_cast<std::uint8_t>(nations))), static_cast<std::uint16_t>(type.anti_personnel),
                                                      static_cast<std::uint16_t>(type.anti_tank), static_cast<std::uint16_t>(type.defense), static_cast<std::uint16_t>(type.speed),
                                                      static_cast<std::uint16_t>(static_cast<unsigned>(type.fuel_economy * type.fuel_capacity) / 10), static_cast<std::uint16_t>(type.gun_range) });
    DrawRowText(game, window, layout, entry, text);
  }
  opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), TABLE_TEXT_COLOR);
}
