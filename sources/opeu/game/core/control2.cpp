// Turn control of a side the player does not command through the menu: when the side is shown, the map moves to the
// current unit and the commander's context is refreshed.
#include "core/control2.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/aiops5.hpp"
#include "core/context.hpp"
#include "link/glue.hpp"
#include "map/mappos.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

// Services leave the module through event; the context runs through its own context_event.
enum Service : std::uint16_t {
  LOCATION = 0, // (x, y, side)
  FINISH   = 1, // ()
};

} // namespace

void opeu_turn_control_other(Game& game, HostEvent event, HostEvent context_event) {
  auto&      data{ Data(game) };
  auto const side{ data.current_side };
  if (data.side_status[side] == SIDE_PLAYED) {
    auto const& unit{ Record<Unit>(data, data.current_unit_ptr) };
    Call(game, event, LOCATION, unit.x, unit.y, side);
    data.context_side = NO_SIDE;
    opeu_daily_context(game, context_event, data.current_side);
  }
  Call(game, event, FINISH);
}

uint16_t svc_control2(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  switch (id) {
  case LOCATION: opeu_arrival_location(game, static_cast<std::uint8_t>(Arg(args, count, 0)), static_cast<std::uint8_t>(Arg(args, count, 1)), static_cast<std::uint8_t>(Arg(args, count, 2))); break;
  case FINISH:   opeu_control_other_finish(game); break;
  default:       port_unhandled("control2", id, count);
  }
  return 0;
}
