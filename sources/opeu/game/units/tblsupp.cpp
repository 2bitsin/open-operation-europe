// The row-draw callback of the city supply table: fuel, ammunition and food stocks with their percentages, the
// garrison strength and its limit. Own cities show their stocks when eligible, enemy cities only when reconnoitred.
#include "units/tblsupp.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/table-rows.hpp"
#include "core/strings.hpp"
#include "core/tblgroup.hpp"
#include "map/objeleg.hpp"
#include "map/query.hpp"
#include "types.hpp"
#include "units/objpct.hpp"
#include "units/table.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// Where the row names start, in columns into the window.
constexpr std::uint16_t NAME_INDENT{ 3 };
} // namespace

void opeu_tbl_draw_supplies(Game& game, HostEvent event, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&               data{ Data(game) };
  ServiceLocals const buffers{ TABLE_SCRATCH };
  buffers.Clear(game);
  auto const  scratch{ buffers.At(0) };
  auto const  text{ static_cast<std::uint16_t>(scratch + ROW_TEXT) };
  auto const& window{ ActiveWindow(data) };
  auto const  layout{ Columns(game, event, column, row) };
  if (flags & DRAW_HEADING) DrawHeading(game, event, window, layout, text, Near(data, data.tbl_supply_header));
  for (auto entry{ first }; entry <= last; ++entry) {
    auto const  near{ data.tbl_objective_list[entry] };
    auto const& city{ Record<Objective>(data, near) };
    auto const  side{ opeu_objective_side(game, near) };
    if (flags & DRAW_NAMES) {
      StartRowName(game, event, window, entry, NAME_INDENT, side);
      Call(game, event, TableService::PRINT, Near(data, data.tbl_supply_name), near);
    }
    if ((side == data.current_side && opeu_objective_eligible(game, near)) || (side != data.current_side && (city.flags & Bits(ObjectiveFlag::MARKED)))) {
      std::uint16_t ammunition{};
      std::uint16_t fuel{};
      std::uint16_t food{};
      opeu_objective_resource_percentages(game, near, &ammunition, &fuel, &food);
      Call(game, event, TableService::FORMAT, text, Near(data, data.tbl_supply_row), city.resources[1], fuel, city.resources[0], ammunition, city.resources[2], food, city.strength,
           city.strength_limit);
    } else
      opeu_string_copy(game, text, Near(data, data.tbl_supply_blank));
    DrawRowText(game, event, window, layout, entry, text);
  }
  Call(game, event, TableService::TEXT_COLOR, TABLE_TEXT_COLOR);
}

// The table's calls are the row-draw services.
uint16_t svc_tblsupp(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_table(game, "tblsupp", id, args, count);
}

auto port_routine_opeu_tbl_draw_supplies(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return RunTableRows(game, svc_tblsupp, opeu_tbl_draw_supplies, args, count);
}
