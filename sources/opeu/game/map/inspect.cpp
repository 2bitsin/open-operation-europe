// Highlights an inspected map cell in whichever map is up: the tactical view or the strategic overview.
#include "map/inspect.hpp"
#include "common/data-segment.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"
#include "map/highlite.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"

using namespace opeu::game;

namespace {
enum class InspectService : uint16_t {
  HIGHLIGHT_VIEW     = 0, // x, y
  HIGHLIGHT_OVERVIEW = 1, // x, y
};
} // namespace

void opeu_map_inspect(Game& game, HostEvent event, uint8_t x, uint8_t y) {
  Call(game, event, Data(game).view_side != NO_SIDE ? InspectService::HIGHLIGHT_VIEW : InspectService::HIGHLIGHT_OVERVIEW, x, y);
}

uint16_t svc_inspect(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  auto const x{ static_cast<uint8_t>(Arg(args, count, 0)) };
  auto const y{ static_cast<uint8_t>(Arg(args, count, 1)) };
  switch (static_cast<InspectService>(id)) {
  case InspectService::HIGHLIGHT_VIEW:     opeu_map_highlight_view(game, svc_highlite, x, y); break;
  case InspectService::HIGHLIGHT_OVERVIEW: opeu_map_highlight_overview(game, svc_highlite, x, y); break;
  default:                                 port_unhandled("inspect", id, count);
  }
  return 0;
}
