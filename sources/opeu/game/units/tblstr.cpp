// The row-draw callback of the item strength table: the type name in its nation's colour, strength and damage.
#include "units/tblstr.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/table-rows.hpp"
#include "core/sideutil.hpp"
#include "core/tblgroup.hpp"
#include "types.hpp"
#include "units/table.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// Where the row names start, in columns into the window.
constexpr std::uint16_t NAME_INDENT{ 2 };
} // namespace

void opeu_tbl_draw_strength(Game& game, HostEvent event, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&               data{ Data(game) };
  ServiceLocals const buffers{ TABLE_SCRATCH };
  buffers.Clear(game);
  auto const  scratch{ buffers.At(0) };
  auto const  text{ static_cast<std::uint16_t>(scratch + ROW_TEXT) };
  auto const& window{ ActiveWindow(data) };
  auto const  layout{ Columns(game, event, column, row) };
  if (flags & DRAW_HEADING) DrawHeading(game, event, window, layout, text, Near(data, data.tbl_str_header));
  for (auto entry{ first }; entry <= last; ++entry) {
    auto const& item{ Record<Item>(data, data.strength_items[entry]) };
    auto const& type{ data.element_types[item.type] };
    if (flags & DRAW_NAMES) {
      StartRowName(game, event, window, entry, NAME_INDENT, opeu_record_side(game, Near(data, type)));
      Call(game, event, TableService::PRINT, Near(data, data.tbl_str_name), type.name);
    }
    Call(game, event, TableService::FORMAT, text, Near(data, data.tbl_str_row), item.quantity, item.extra);
    DrawRowText(game, event, window, layout, entry, text);
  }
  Call(game, event, TableService::TEXT_COLOR, TABLE_TEXT_COLOR);
}

// The table's calls are the row-draw services.
uint16_t svc_tblstr(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_table(game, "tblstr", id, args, count);
}

auto port_routine_opeu_tbl_draw_strength(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return RunTableRows(game, svc_tblstr, opeu_tbl_draw_strength, args, count);
}
