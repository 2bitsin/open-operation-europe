// The inspect highlight on a map cell: in the tactical view the cell frame's mask (tile 3) and image (tile 2), in the
// overview the 1 x 8 cursor marker.
#include "map/highlite.hpp"
#include "common/data-segment.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"
#include "map/blast.hpp"
#include "map/mappixel.hpp"
#include "types.hpp"

#include "port.hpp"

using namespace opeu::game;

namespace {
constexpr uint16_t MASK_MODE{ 0x20f };
constexpr uint16_t IMAGE_MODE{ 0x307 };
constexpr uint16_t MARKER_MODE{ 7 };
} // namespace

// frame: this routine's frame; the pixel position is computed in its locals.
void opeu_map_highlight_view(uint8_t* ds, HostEvent event, uint16_t frame, uint8_t x, uint8_t y) {
  auto&      data{ Data(ds) };
  auto const pixel{ CellPixel(ds, Local(frame, 2), Local(frame, 4), x, y, opeu_view_pixel) };
  auto       tile{ StreamFrameTile(data, 3) };
  Call(ds, event, MapService::DRAW_FAR, pixel.x, pixel.y, 3, 24, MASK_MODE, tile.segment, tile.offset);
  tile = StreamFrameTile(data, 2);
  Call(ds, event, MapService::DRAW_FAR, pixel.x, pixel.y, 3, 24, IMAGE_MODE, tile.segment, tile.offset);
}

void opeu_map_highlight_overview(uint8_t* ds, HostEvent event, uint16_t frame, uint8_t x, uint8_t y) {
  auto const pixel{ CellPixel(ds, Local(frame, 2), Local(frame, 4), x, y, opeu_overview_pixel) };
  Call(ds, event, MapService::DRAW_NEAR, pixel.x, pixel.y, 1, 8, MARKER_MODE, Near(Data(ds), Data(ds).flash_marker));
}

// Only the far and near draws reach this module's service.
uint16_t svc_highlite(uint8_t* ds, uint16_t kind, const uint16_t* args, uint16_t count) {
  switch (static_cast<MapService>(kind)) {
  case MapService::DRAW_FAR:
  case MapService::DRAW_NEAR: return svc_map_plan(ds, "highlite", kind, args, count);
  default:                    port_unhandled("highlite", kind, count);
  }
  return 0;
}
