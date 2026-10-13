// The row-draw callback of the city supply table: fuel, ammunition and food stocks with their percentages, the
// garrison strength and its limit. Own cities show their stocks when eligible, enemy cities only when reconnoitred.
#include "units/tblsupp.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/table-rows.hpp"
#include "core/strings.hpp"
#include "core/tblgroup.hpp"
#include "core/textout.hpp"
#include "map/objeleg.hpp"
#include "map/query.hpp"
#include "types.hpp"
#include "units/objpct.hpp"
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

void opeu_tbl_draw_supplies(Game& game, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&               data{ Data(game) };
  ServiceLocals const buffers{ TABLE_SCRATCH };
  buffers.Clear(game);
  auto const  scratch{ buffers.At(0) };
  auto const  text{ static_cast<std::uint16_t>(scratch + ROW_TEXT) };
  auto const& window{ ActiveWindow(data) };
  auto const  layout{ TableColumnLayout(game, column, row) };
  if (flags & DRAW_HEADING) DrawHeading(game, window, layout, text, Near(data, data.tbl_supply_header));
  for (auto entry{ first }; entry <= last; ++entry) {
    auto const  near{ data.tbl_objective_list[entry] };
    auto const& city{ Record<Objective>(data, near) };
    auto const  side{ opeu_objective_side(game, near) };
    if (flags & DRAW_NAMES) {
      StartRowName(game, window, entry, NAME_INDENT, side);
      opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.tbl_supply_name)), static_cast<std::uint16_t>(near) });
    }
    if ((side == data.current_side && opeu_objective_eligible(game, near)) || (side != data.current_side && (city.flags & Bits(ObjectiveFlag::MARKED)))) {
      std::uint16_t ammunition{};
      std::uint16_t fuel{};
      std::uint16_t food{};
      opeu_objective_resource_percentages(game, near, &ammunition, &fuel, &food);
      opeu_request_format(game, std::array<std::uint16_t, 10>{ static_cast<std::uint16_t>(text), static_cast<std::uint16_t>(Near(data, data.tbl_supply_row)),
                                                               static_cast<std::uint16_t>(city.resources[1]), static_cast<std::uint16_t>(fuel), static_cast<std::uint16_t>(city.resources[0]),
                                                               static_cast<std::uint16_t>(ammunition), static_cast<std::uint16_t>(city.resources[2]), static_cast<std::uint16_t>(food),
                                                               static_cast<std::uint16_t>(city.strength), static_cast<std::uint16_t>(city.strength_limit) });
    } else
      opeu_string_copy(game, text, Near(data, data.tbl_supply_blank));
    DrawRowText(game, window, layout, entry, text);
  }
  opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), TABLE_TEXT_COLOR);
}

auto port_routine_opeu_tbl_draw_supplies(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return RunTableRows(game, opeu_tbl_draw_supplies, args, count);
}
