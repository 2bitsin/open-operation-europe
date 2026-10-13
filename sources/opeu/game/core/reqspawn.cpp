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

namespace {

// The services opeu_request_action2 calls through its event.
enum class SpawnService : uint16_t {
  PREPARE  = 1, // opeu_arrival_prepare(unit)
  CONTEXT  = 2, // opeu_set_turn_context(value)
  LOCATION = 3, // opeu_arrival_location(x, y, side)
  VIEW     = 4, // opeu_arrival_view(unit)
  SHOW     = 5, // opeu_daily_show(id, side, present offset, present segment)
  FINISH   = 6, // opeu_finish_side_initialization(value)
};

} // namespace

// Builds the requested regiment (arrival_unit) and readies it on the map; a person playing the side sees it arrive.
void opeu_request_action2(Game& game, PendingEvent event, uint8_t side, uint16_t) {
  auto& data{ Data(game) };
  data.arrival_unit = opeu_build_request_unit(game, side);
  CallWords(game, event, SpawnService::PREPARE, data.arrival_unit);
  if (data.side_status[side] != SIDE_PLAYED) return;
  CallWords(game, event, SpawnService::CONTEXT, ARRIVAL_CONTEXT);
  auto const& unit{ Record<Unit>(data, data.arrival_unit) };
  CallWords(game, event, SpawnService::LOCATION, unit.x, unit.y, side);
  CallWords(game, event, SpawnService::VIEW, data.arrival_unit);
  data.arrival_side = side;
  CallWords(game, event, SpawnService::SHOW, 0, side, ARRIVAL_PRESENT_OFFSET, ARRIVAL_PRESENT_SEGMENT);
  CallWords(game, event, SpawnService::FINISH, 0);
}

uint16_t svc_reqspawn(Game& game, uint16_t id, uint16_t a, uint16_t b, uint16_t c, uint16_t d) {
  switch (static_cast<SpawnService>(id)) {
  case SpawnService::PREPARE: {
    opeu_arrival_prepare(game, a);
    break;
  }
  case SpawnService::CONTEXT:  opeu_set_turn_context(game, svc_sound, (uint8_t)a); break;
  case SpawnService::LOCATION: {
    opeu_arrival_location(game, (uint8_t)a, (uint8_t)b, (uint8_t)c);
    break;
  }
  case SpawnService::VIEW: {
    opeu_arrival_view(game, a);
    break;
  }
  case SpawnService::SHOW:   opeu_daily_show(game, (const DialogHost*)port_dialogs_host(), a, (uint8_t)b, (uint32_t)d << 16 | c); break;
  case SpawnService::FINISH: opeu_finish_side_initialization(game, svc_sound, (uint8_t)a); break;
  default:                   port_unhandled("reqspawn", id, 4);
  }
  return 0;
}
