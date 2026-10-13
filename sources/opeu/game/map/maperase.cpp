// Redrawing a map cell in whichever map is up, placing or removing a unit and redrawing its cell, and switching the
// overview to a side.
#include "map/maperase.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "map/blast.hpp"
#include "map/mapframe.hpp"
#include "map/mapover.hpp"
#include "map/mapview.hpp"
#include "map/occupy.hpp"
#include "types.hpp"

#include <ranges>

using namespace opeu::game;

namespace {
constexpr int16_t VIEW_LAST{ 11 }; // the view shows 12 x 12 cells

void EraseUnitCell(Game& game, uint16_t unit) {
  auto const& record{ Record<Unit>(Data(game), unit) };
  opeu_map_erase(game, record.x, record.y);
}
} // namespace

// Repaints the cell in the overview, or in the tactical view when the cell lies inside it. frame: this routine's frame.
void opeu_map_erase(Game& game, uint8_t x, uint8_t y) {
  auto const& data{ Data(game) };
  auto const  view_x{ static_cast<int16_t>(data.viewport_x) };
  auto const  view_y{ static_cast<int16_t>(data.viewport_y) };
  if (data.overview_side != NO_SIDE)
    opeu_overview_cell_draw(game, x, y);
  else if (data.view_side != NO_SIDE && x >= view_x && x <= static_cast<int16_t>(view_x + VIEW_LAST) && y >= view_y && y <= static_cast<int16_t>(view_y + VIEW_LAST))
    opeu_view_cell_draw(game, x, y);
}

void opeu_arrival_prepare(Game& game, uint16_t unit) {
  opeu_place_unit(game, unit);
  EraseUnitCell(game, unit);
}

void opeu_unit_remove_draw(Game& game, uint16_t unit) {
  opeu_remove_unit(game, unit);
  EraseUnitCell(game, unit);
}

// Switches the overview to a player's side not shown yet and repaints every cell and the rulers.
void opeu_daily_activate(Game& game, uint8_t side) {
  auto& data{ Data(game) };
  if (data.side_status[side] != SIDE_PLAYED || data.overview_side == side) return;
  data.view_side     = NO_SIDE;
  data.overview_side = side;
  opeu_turn_frame(game, static_cast<uint8_t>(0));
  for (auto const y : std::views::iota(uint8_t{ 0 }, MAP_HEIGHT))
    for (auto const x : std::views::iota(uint8_t{ 0 }, MAP_WIDTH)) opeu_map_erase(game, x, y);
  opeu_overview_frame(game);
}
