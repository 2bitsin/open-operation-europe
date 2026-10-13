// Redrawing a map cell in whichever map is up, placing or removing a unit and redrawing its cell, and switching the
// overview to a side.
#include "map/maperase.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "map/blast.hpp"
#include "map/mapframe.hpp"
#include "map/mapover.hpp"
#include "map/mapview.hpp"
#include "map/occupy.hpp"
#include "types.hpp"

using namespace opeu::game;

namespace {
constexpr int16_t VIEW_LAST{ 11 }; // the view shows 12 x 12 cells

void EraseUnitCell(Game& game, HostEvent event, uint16_t unit) {
  auto const& record{ Record<Unit>(Data(game), unit) };
  opeu_map_erase(game, event, record.x, record.y);
}
} // namespace

// Repaints the cell in the overview, or in the tactical view when the cell lies inside it. frame: this routine's frame.
void opeu_map_erase(Game& game, HostEvent event, uint8_t x, uint8_t y) {
  auto const& data{ Data(game) };
  auto const  view_x{ static_cast<int16_t>(data.viewport_x) };
  auto const  view_y{ static_cast<int16_t>(data.viewport_y) };
  if (data.overview_side != NO_SIDE)
    opeu_overview_cell_draw(game, event, x, y);
  else if (data.view_side != NO_SIDE && x >= view_x && x <= static_cast<int16_t>(view_x + VIEW_LAST) && y >= view_y && y <= static_cast<int16_t>(view_y + VIEW_LAST))
    opeu_view_cell_draw(game, event, x, y);
}

void opeu_arrival_prepare(Game& game, HostEvent event, uint16_t unit) {
  opeu_place_unit(game, unit);
  EraseUnitCell(game, event, unit);
}

void opeu_unit_remove_draw(Game& game, HostEvent event, uint16_t unit) {
  opeu_remove_unit(game, unit);
  EraseUnitCell(game, event, unit);
}

// Switches the overview to a player's side not shown yet and repaints every cell and the rulers.
void opeu_daily_activate(Game& game, HostEvent event, uint8_t side) {
  auto& data{ Data(game) };
  if (data.side_status[side] != SIDE_PLAYED || data.overview_side == side) return;
  data.view_side     = NO_SIDE;
  data.overview_side = side;
  Call(game, event, MapService::STATUS_MODE, 0);
  for (uint8_t y{ 0 }; y < MAP_HEIGHT; y++)
    for (uint8_t x{ 0 }; x < MAP_WIDTH; x++) opeu_map_erase(game, event, x, y);
  opeu_overview_frame(game, event);
}

uint16_t svc_maperase(Game& game, uint16_t kind, const uint16_t* args, uint16_t count) {
  return svc_map_plan(game, "maperase", kind, args, count);
}
