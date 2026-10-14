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
#include "port.hpp"
#include "services.hpp"
#include <array>

using namespace opeu::game;

namespace {
// Where the row names start, in columns into the window.
constexpr std::uint16_t NAME_INDENT{ 3 };
} // namespace

void opeu_tbl_draw_supplies(Game& game, DrawRequest const& request) {
  auto const [column, first, last_column, last, flags]{ request };
  auto&                                data{ Data(game) };
  TextBuffer<TABLE_SCRATCH - ROW_TEXT> text;
  auto const&                          window{ ActiveWindow(data) };
  auto const                           layout{ TableColumnLayout(game, column, last_column) };
  if (flags & DRAW_HEADING) DrawHeading(game, window, layout, text, Near(data, data.tbl_supply_header));
  for (auto const entry : DrawnRows(first, last)) {
    auto const  near{ data.tbl_objective_list[entry] };
    auto const& city{ Record<Objective>(data, near) };
    auto const  side{ opeu_objective_side(game, near) };
    if (flags & DRAW_NAMES) {
      StartRowName(game, window, entry, NAME_INDENT, side);
      opeu_cursor_print(game, std::array<FormatArgument, 2>{ Near(data, data.tbl_supply_name), near });
    }
    if ((side == data.current_side && opeu_objective_eligible(game, near)) || (side != data.current_side && (city.flags & Bits(ObjectiveFlag::MARKED)))) {
      auto const percentages{ ObjectiveResourcePercentages(data, near) };
      opeu_request_format(game, text,
                          std::array<FormatArgument, 9>{ Near(data, data.tbl_supply_row), city.resources[1], percentages.fuel, city.resources[0], percentages.ammunition, city.resources[2],
                                                         percentages.food, city.strength, city.strength_limit });
    } else
      opeu_string_copy(game, text, Near(data, data.tbl_supply_blank));
    DrawRowText(game, window, layout, entry, text);
  }
  opeu_text_color(game, TABLE_TEXT_COLOR);
}
