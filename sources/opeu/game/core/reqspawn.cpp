// Delivering a regiment request: the regiment is built and, for a person's side, shown arriving.
#include "core/reqspawn.hpp"
#include "types.hpp"

#include "common/arrival.hpp"
#include "common/memory.hpp"
#include "core/requnit.hpp"
#include "data-segment.hpp"

#include "common/game.hpp"
#include "core/dialogs.hpp"
#include "io/sound.hpp"
#include "link/glue.hpp"
#include "map/maperase.hpp"
#include "map/mappos.hpp"
#include "map/mapunit.hpp"
#include "port.hpp"
#include "services.hpp"

#include <utility>

using namespace opeu::game;

namespace { } // namespace

// Builds the requested regiment (arrival_unit) and readies it on the map; a person playing the side sees it arrive.
void opeu_request_action2(Game& game, uint8_t side, uint16_t) {
  auto& data{ Data(game) };
  data.arrival_unit = opeu_build_request_unit(game, side);
  opeu_arrival_prepare(game, data.arrival_unit);
  if (data.side_status[side] != SIDE_PLAYED) return;
  opeu_set_turn_context(game, ARRIVAL_CONTEXT);
  auto const& unit{ Record<Unit>(data, data.arrival_unit) };
  opeu_arrival_location(game, unit.x, unit.y, side);
  opeu_arrival_view(game, data.arrival_unit);
  data.arrival_side = side;
  opeu_daily_show(game, static_cast<const DialogHost*>(port_dialogs_host()), 0, side, Presenter::ARRIVAL);
  opeu_finish_side_initialization(game, 0);
}
