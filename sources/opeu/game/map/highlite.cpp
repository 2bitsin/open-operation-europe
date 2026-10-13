// The inspect highlight on a map cell: in the tactical view the cell frame's mask (tile 3) and image (tile 2), in the
// overview the 1 x 8 cursor marker.
#include "map/highlite.hpp"
#include "common/data-segment.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"
#include "map/blast.hpp"
#include "map/mappixel.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"

using namespace opeu::game;

namespace {
constexpr uint16_t MASK_MODE{ 0x20f };
constexpr uint16_t IMAGE_MODE{ 0x307 };
constexpr uint16_t MARKER_MODE{ 7 };
} // namespace

void opeu_map_highlight_view(Game& game, HostEvent event, uint8_t x, uint8_t y) {
  auto&      data{ Data(game) };
  auto const pixel{ ViewPixel(data, { x, y }) };
  auto       tile{ StreamFrameTile(data, 3) };
  Call(game, event, MapService::DRAW_FAR, pixel.x, pixel.y, 3, 24, MASK_MODE, tile.segment, tile.offset);
  tile = StreamFrameTile(data, 2);
  Call(game, event, MapService::DRAW_FAR, pixel.x, pixel.y, 3, 24, IMAGE_MODE, tile.segment, tile.offset);
}

void opeu_map_highlight_overview(Game& game, HostEvent event, uint8_t x, uint8_t y) {
  auto const pixel{ OverviewPixel({ x, y }) };
  Call(game, event, MapService::DRAW_NEAR, pixel.x, pixel.y, 1, 8, MARKER_MODE, Near(Data(game), Data(game).flash_marker));
}

// Only the far and near draws reach this module's service.
uint16_t svc_highlite(Game& game, uint16_t kind, const uint16_t* args, uint16_t count) {
  switch (static_cast<MapService>(kind)) {
  case MapService::DRAW_FAR:
  case MapService::DRAW_NEAR: return svc_map_plan(game, "highlite", kind, args, count);
  default:                    port_unhandled("highlite", kind, count);
  }
  return 0;
}
