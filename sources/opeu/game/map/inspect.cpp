// Highlights an inspected map cell in whichever map is up: the tactical view or the strategic overview.
#include "map/inspect.hpp"
#include "common/data-segment.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"
#include "map/highlite.hpp"
#include "types.hpp"

#include "port.hpp"

using namespace opeu::game;

namespace {
enum class InspectService : uint16_t {
  HIGHLIGHT_VIEW     = 0, // x, y
  HIGHLIGHT_OVERVIEW = 1, // x, y
};
constexpr uint8_t NO_SIDE{ 255 };
} // namespace

void opeu_map_inspect(uint8_t* ds, HostEvent event, uint8_t x, uint8_t y) {
  Call(ds, event, Data(ds).view_side != NO_SIDE ? InspectService::HIGHLIGHT_VIEW : InspectService::HIGHLIGHT_OVERVIEW,
       x, y);
}

uint16_t svc_inspect(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  auto const x{ static_cast<uint8_t>(Arg(args, count, 0)) };
  auto const y{ static_cast<uint8_t>(Arg(args, count, 1)) };
  switch (static_cast<InspectService>(id)) {
  case InspectService::HIGHLIGHT_VIEW:
    WithMapFrame([&](uint16_t frame) { opeu_map_highlight_view(ds, svc_highlite, frame, x, y); });
    break;
  case InspectService::HIGHLIGHT_OVERVIEW:
    WithMapFrame([&](uint16_t frame) { opeu_map_highlight_overview(ds, svc_highlite, frame, x, y); });
    break;
  default: port_unhandled("inspect", id, count);
  }
  return 0;
}
