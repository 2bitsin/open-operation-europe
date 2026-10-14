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
#include "port.hpp"
#include <array>

using namespace opeu::game;

namespace {
constexpr uint16_t NAME_BUFFER_BYTES{ 38 }; // the DS reserve the unit's name is formatted into
} // namespace

// x and y: the cell (their low bytes).
void opeu_map_cursor(Game& game, uint16_t x, uint16_t y) {
  auto const& data{ Data(game) };
  auto const  cell_x{ static_cast<uint8_t>(x) };
  auto const  cell_y{ static_cast<uint8_t>(y) };
  auto const  terrain{ opeu_map_code(game, cell_x, cell_y) };
  opeu_turn_active_begin(game);
  opeu_cursor_print(game, std::array<FormatArgument, 3>{ Near(data, data.cursor_position_format), data.map_scale * cell_x, data.map_scale * cell_y });
  opeu_ui_text(game, terrain == Bits(Terrain::CITY) ? opeu_find_objective_at(game, cell_x, cell_y) : data.terrain_names[terrain]);
  auto const side{ opeu_cell_side(game, cell_x, cell_y) };
  if (side != NO_SIDE) {
    auto const unit{ opeu_find_unit_at(game, cell_x, cell_y) };
    if ((side == data.current_side && opeu_unit_selection_eligible(game, unit)) || (side != data.current_side && opeu_unit_special(game, unit))) {
      ServiceLocals const locals{ NAME_BUFFER_BYTES };
      auto const          name{ opeu_unit_name(game, unit, locals.At(0)) };
      opeu_cursor_print(game, std::array<FormatArgument, 3>{ Near(data, data.cursor_unit_format), data.side_names[side], name });
    }
  }
  opeu_ui_text(game, Near(data, data.cursor_end));
}
