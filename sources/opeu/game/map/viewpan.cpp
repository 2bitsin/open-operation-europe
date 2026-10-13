// Pans the tactical view by a step: the cursor cell is erased first, then the view scrolls to its origin plus the step.
#include "map/viewpan.hpp"
#include "common/data-segment.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"
#include "map/maperase.hpp"
#include "map/mapscrol.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"

using namespace opeu::game;

namespace {
enum class PanService : uint16_t {
  ERASE     = 0, // x, y
  SCROLL_TO = 1, // x, y, side
};
} // namespace

void opeu_viewport_pan(Game& game, HostEvent event, int16_t dx, int16_t dy, uint8_t x, uint8_t y, uint8_t side) {
  auto const& data{ Data(game) };
  Call(game, event, PanService::ERASE, x, y);
  Call(game, event, PanService::SCROLL_TO, static_cast<int16_t>(data.viewport_x) + dx, static_cast<int16_t>(data.viewport_y) + dy, side);
}

uint16_t svc_viewpan(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  switch (static_cast<PanService>(id)) {
  case PanService::ERASE: opeu_map_erase(game, svc_maperase, static_cast<uint8_t>(Arg(args, count, 0)), static_cast<uint8_t>(Arg(args, count, 1))); break;
  case PanService::SCROLL_TO:
    opeu_map_scroll_to(game, svc_mapscrol, static_cast<int16_t>(Arg(args, count, 0)), static_cast<int16_t>(Arg(args, count, 1)), static_cast<uint8_t>(Arg(args, count, 2)));
    break;
  default: port_unhandled("viewpan", id, count);
  }
  return 0;
}
