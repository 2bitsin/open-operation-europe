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
#include "types.hpp"
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

void opeu_tbl_draw_types(Game& game, HostEvent event, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&               data{ Data(game) };
  ServiceLocals const buffers{ TABLE_SCRATCH };
  buffers.Clear(game);
  auto const  scratch{ buffers.At(0) };
  auto const  text{ static_cast<std::uint16_t>(scratch + ROW_TEXT) };
  auto const& window{ ActiveWindow(data) };
  auto const  layout{ Columns(game, event, column, row) };
  if (flags & DRAW_HEADING) DrawHeading(game, event, window, layout, text, Near(data, data.tbl_type_header));
  for (auto entry{ first }; entry <= last; ++entry) {
    auto const  near{ data.tbl_type_list[entry] };
    auto const& type{ Record<ElementType>(data, near) };
    if (flags & DRAW_NAMES) {
      StartRowName(game, event, window, entry, NAME_INDENT, opeu_record_side(game, near));
      Call(game, event, TableService::PRINT, Near(data, data.tbl_type_name), type.name);
    }
    auto const    general{ Record<Person>(data, data.context_record1).status };
    std::uint16_t nations{ !(type.sides & general) ? type.sides : general };
    Call(game, event, TableService::FORMAT, text, Near(data, data.tbl_type_row), opeu_first_mask_name(game, static_cast<std::uint8_t>(nations)), type.anti_personnel, type.anti_tank, type.defense,
         type.speed, static_cast<unsigned>(type.fuel_economy * type.fuel_capacity) / 10, type.gun_range);
    DrawRowText(game, event, window, layout, entry, text);
  }
  Call(game, event, TableService::TEXT_COLOR, TABLE_TEXT_COLOR);
}

// The table's calls are the row-draw services.
uint16_t svc_tbltype(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_table(game, "tbltype", id, args, count);
}

auto port_routine_opeu_tbl_draw_types(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return RunTableRows(game, svc_tbltype, opeu_tbl_draw_types, args, count);
}
