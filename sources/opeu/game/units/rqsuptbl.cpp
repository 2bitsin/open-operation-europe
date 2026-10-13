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
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>

using namespace opeu::game;

namespace { } // namespace

// Draws rows first..last of the type table's `column`, with the heading and the type names as `flags` ask. The
// nation shown is the general's (context_record1) when the type is fielded by it, else the type's first nation.
void opeu_rq_draw_types(Game& game, HostEvent event, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&               data{ Data(game) };
  ServiceLocals const buffers{ TABLE_SCRATCH };
  buffers.Clear(game);
  auto const  scratch{ buffers.At(0) };
  auto const  text{ static_cast<std::uint16_t>(scratch + ROW_TEXT) };
  auto const& window{ ActiveWindow(data) };
  auto const  layout{ TableColumnLayout(game, column, row) };
  if (flags & DRAW_HEADING) {
    opeu_request_format(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(text), static_cast<std::uint16_t>(Near(data, data.rq_type_header)) });
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(ColumnX(data, window, layout, -0x20)), static_cast<int16_t>(window.y + 1));
    opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), SideTextColor(data.current_side));
    opeu_text_put_range(game, static_cast<TextHost*>(port_textout_host()), text, static_cast<uint8_t>(layout.start), static_cast<uint8_t>(layout.count));
  }
  for (auto entry{ first }; entry <= last; ++entry) {
    auto const& type{ data.element_types[data.rq_type_list[entry]] };
    auto const  y{ RowY(data, window, entry) };
    if (flags & DRAW_NAMES) {
      opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(window.x + 3), static_cast<int16_t>(y));
      opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), SideTextColor(data.current_side));
      opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.rq_type_name)), static_cast<std::uint16_t>(type.name) });
    }
    auto const          general{ Record<Person>(data, data.context_record1).status };
    std::uint16_t const nations{ !(type.sides & general) ? type.sides : general };
    opeu_request_format(game,
                        std::array<std::uint16_t, 9>{ static_cast<std::uint16_t>(text), static_cast<std::uint16_t>(Near(data, data.rq_type_row)),
                                                      static_cast<std::uint16_t>(opeu_first_mask_name(game, nations)), static_cast<std::uint16_t>(type.anti_personnel),
                                                      static_cast<std::uint16_t>(type.anti_tank), static_cast<std::uint16_t>(type.defense), static_cast<std::uint16_t>(type.speed),
                                                      static_cast<std::uint16_t>(static_cast<unsigned>(type.fuel_economy * type.fuel_capacity) / 10), static_cast<std::uint16_t>(type.gun_range) });
    DrawRowText(game, window, layout, entry, text);
  }
  opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), TABLE_TEXT_COLOR);
}

// The services of the type table: the row-draw services.
uint16_t svc_rqsuptbl(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_table(game, "rqsuptbl", id, args, count);
}

auto port_routine_opeu_rq_draw_types(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return RunTableRows(game, svc_rqsuptbl, opeu_rq_draw_types, args, count);
}
