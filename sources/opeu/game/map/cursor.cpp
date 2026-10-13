// The map cursor's status line: the cell's coordinates in map scale, the objective's name or the terrain, then the
// side and name of a unit there when the current side may see it.
#include "map/cursor.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/textout.hpp"
#include "map/grid.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/stats.hpp"
#include "units/unitname.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"

using namespace opeu::game;

namespace {
enum class CursorService : uint16_t {
  BEGIN  = 0, // start the status line
  TEXT   = 1, // text
  FORMAT = 2, // buffer, format, name, number (raised by opeu_unit_name)
  PRINT  = 3, // format and its two argument words
};
constexpr uint16_t NAME_BUFFER_BYTES{ 38 }; // the DS reserve the unit's name is formatted into
} // namespace

// x and y: the cell (their low bytes).
void opeu_map_cursor(Game& game, HostEvent event, uint16_t x, uint16_t y) {
  auto const& data{ Data(game) };
  auto const  cell_x{ static_cast<uint8_t>(x) };
  auto const  cell_y{ static_cast<uint8_t>(y) };
  auto const  terrain{ opeu_map_code(game, cell_x, cell_y) };
  Call(game, event, CursorService::BEGIN);
  Call(game, event, CursorService::PRINT, Near(data, data.cursor_position_format), data.map_scale * cell_x, data.map_scale * cell_y);
  Call(game, event, CursorService::TEXT, terrain == Bits(Terrain::CITY) ? opeu_find_objective_at(game, cell_x, cell_y) : data.terrain_names[terrain]);
  auto const side{ opeu_cell_side(game, cell_x, cell_y) };
  if (side != NO_SIDE) {
    auto const unit{ opeu_find_unit_at(game, cell_x, cell_y) };
    if ((side == data.current_side && opeu_unit_selection_eligible(game, unit)) || (side != data.current_side && opeu_unit_special(game, unit))) {
      ServiceLocals const locals{ NAME_BUFFER_BYTES };
      auto const          name{ opeu_unit_name(game, event, unit, locals.At(0)) };
      Call(game, event, CursorService::PRINT, Near(data, data.cursor_unit_format), data.side_names[side], name);
    }
  }
  Call(game, event, CursorService::TEXT, Near(data, data.cursor_end));
}

uint16_t svc_cursor(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  switch (static_cast<CursorService>(id)) {
  case CursorService::BEGIN:  opeu_turn_active_begin(game, static_cast<DialogHost const*>(port_dialogs_host())); break;
  case CursorService::TEXT:   opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), Arg(args, count, 0)); break;
  case CursorService::FORMAT: opeu_request_format(game, { args, count }); break;
  case CursorService::PRINT:  opeu_cursor_print(game, { args, count }); break;
  default:                    port_unhandled("cursor", id, count);
  }
  return 0;
}
