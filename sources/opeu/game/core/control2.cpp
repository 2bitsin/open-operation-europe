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

namespace { } // namespace

void opeu_turn_control_other(Game& game) {
  auto&      data{ Data(game) };
  auto const side{ data.current_side };
  if (data.side_status[side] == SIDE_PLAYED) {
    auto const& unit{ Record<Unit>(data, data.current_unit_ptr) };
    opeu_arrival_location(game, unit.x, unit.y, side);
    data.context_side = NO_SIDE;
    opeu_daily_context(game, data.current_side);
  }
  opeu_control_other_finish(game);
}
