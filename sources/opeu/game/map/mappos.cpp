// The tactical view's position: dropping the shown view when a cell leaves its centre, centring the view on a cell, and
// showing an arrival either by activating the day's events (no map up) or by centring on its cell.
#include "map/mappos.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "map/blast.hpp"
#include "map/maperase.hpp"
#include "map/mapscrol.hpp"
#include "types.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t BLOCK_MASK{ 0xfc }; // the view moves in blocks of 4 cells
constexpr int16_t BLOCK{ 4 };
} // namespace

// Forgets the shown view unless the block holding (x, y) lies one block inside the viewport origin on both axes.
void opeu_control_position(uint8_t* ds, uint16_t x, uint16_t y) {
  auto& data{ Data(ds) };
  if (static_cast<uint16_t>((x & BLOCK_MASK) - data.viewport_x) != BLOCK || static_cast<uint16_t>((y & BLOCK_MASK) - data.viewport_y) != BLOCK) data.view_side = NO_SIDE;
}

// Scrolls so that the block holding (x, y) is one block inside the view. frame: this routine's frame.
void opeu_map_center(uint8_t* ds, HostEvent event, uint8_t x, uint8_t y, uint8_t side) {
  opeu_map_scroll_to(ds, event, static_cast<int16_t>((x & BLOCK_MASK) - BLOCK), static_cast<int16_t>((y & BLOCK_MASK) - BLOCK), side);
}

void opeu_arrival_location(uint8_t* ds, HostEvent event, uint8_t x, uint8_t y, uint8_t side) {
  if (!Data(ds).map_mode[0])
    opeu_daily_activate(ds, event, side);
  else
    opeu_map_center(ds, event, x, y, side);
}

// This module raises nothing itself; the routines it runs use the map segment's shared services.
uint16_t svc_mappos(uint8_t* ds, uint16_t kind, const uint16_t* args, uint16_t count) {
  return svc_map_plan(ds, "mappos", kind, args, count);
}
