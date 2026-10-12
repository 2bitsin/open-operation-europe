// Pans the tactical view by a step: the cursor cell is erased first, then the view scrolls to its origin plus the step.
#include "types.hpp"
#include "map/viewpan.hpp"
#include "common/data-segment.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"
#include "map/maperase.hpp"
#include "map/mapscrol.hpp"

#include "port.hpp"

using namespace opeu::game;

namespace {
enum class PanService : uint16_t {
  ERASE     = 0, // x, y
  SCROLL_TO = 1, // x, y, side
};
} // namespace

void opeu_viewport_pan(uint8_t* ds, HostEvent event, int16_t dx, int16_t dy,
                       uint8_t x, uint8_t y, uint8_t side) {
  auto const& data{ Data(ds) };
  Call(ds, event, PanService::ERASE, x, y);
  Call(ds, event, PanService::SCROLL_TO,
       static_cast<int16_t>(data.viewport_x) + dx,
       static_cast<int16_t>(data.viewport_y) + dy, side);
}

uint16_t svc_viewpan(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count) {
  switch (static_cast<PanService>(id)) {
    case PanService::ERASE:
      WithMapFrame([&](uint16_t frame) {
        opeu_map_erase(ds, svc_maperase, frame,
                       static_cast<uint8_t>(Arg(args, count, 0)),
                       static_cast<uint8_t>(Arg(args, count, 1)));
      });
      break;
    case PanService::SCROLL_TO:
      WithMapFrame([&](uint16_t frame) {
        opeu_map_scroll_to(ds, svc_mapscrol, frame,
                           static_cast<int16_t>(Arg(args, count, 0)),
                           static_cast<int16_t>(Arg(args, count, 1)),
                           static_cast<uint8_t>(Arg(args, count, 2)));
      });
      break;
    default: port_unhandled("viewpan", id, count);
  }
  return 0;
}
