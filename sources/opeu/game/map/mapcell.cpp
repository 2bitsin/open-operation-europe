// Painting one tactical-view cell frame, and flashing the cursor on a map cell in whichever map is up.
#include "map/mapcell.hpp"
#include "common/data-segment.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"
#include "map/blast.hpp"
#include "map/mappixel.hpp"
#include "types.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t  NO_SIDE{ 255 };
constexpr uint16_t BORDER_MODE{ 0x20f };
constexpr uint16_t IMAGE_MODE{ 0x300 }; // added to the caller's mode
constexpr uint16_t HIGHLIGHT_MODE{ 7 };
} // namespace

// The cell frame's border mask (tile 1), then its image (tile 0) in the given mode, both 3 x 24 at (x, y).
void opeu_view_cell_paint(uint8_t* ds, HostEvent event, uint16_t x, uint16_t y, uint16_t mode) {
  auto& data{ Data(ds) };
  auto  tile{ StreamFrameTile(data, 1) };
  Call(ds, event, MapService::DRAW_FAR, x, y, 3, 24, BORDER_MODE, tile.segment, tile.offset);
  tile = StreamFrameTile(data, 0);
  Call(ds, event, MapService::DRAW_FAR, x, y, 3, 24, mode | IMAGE_MODE, tile.segment, tile.offset);
}

// Overview: alternately draws the cursor marker over the saved 1 x 8 background and restores the background. Tactical
// view: repaints the cell highlighted. frame: this routine's frame; the pixel position is computed in its locals.
void opeu_map_cell_flash(uint8_t* ds, HostEvent event, uint16_t frame, uint8_t x, uint8_t y) {
  auto&      data{ Data(ds) };
  auto const buffer{ Near(data, data.flash_buffers) };
  if (data.overview_side != NO_SIDE) {
    auto const pixel{ CellPixel(ds, Local(frame, 2), Local(frame, 4), x, y, opeu_overview_pixel) };
    if (data.flash_state) {
      Call(ds, event, MapService::RESTORE_NEAR, pixel.x, pixel.y, 1, 8, 0, buffer);
      data.flash_state = 0;
    } else {
      Call(ds, event, MapService::SAVE_NEAR, pixel.x, pixel.y, 1, 8, buffer);
      Call(ds, event, MapService::DRAW_NEAR, pixel.x, pixel.y, 1, 8, HIGHLIGHT_MODE, Near(data, data.flash_marker));
      data.flash_state = 1;
    }
  } else if (data.view_side != NO_SIDE) {
    auto const pixel{ CellPixel(ds, Local(frame, 2), Local(frame, 4), x, y, opeu_view_pixel) };
    opeu_view_cell_paint(ds, event, pixel.x, pixel.y, HIGHLIGHT_MODE);
  }
}

uint16_t svc_mapcell(uint8_t* ds, uint16_t kind, const uint16_t* args, uint16_t count) {
  return svc_map_plan(ds, "mapcell", kind, args, count);
}
