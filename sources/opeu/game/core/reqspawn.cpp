// Delivering a regiment request: the regiment is built and, for a person's side, shown arriving.
#include "core/reqspawn.hpp"
#include "types.hpp"

#include "common/arrival.hpp"
#include "common/memory.hpp"
#include "core/requnit.hpp"
#include "data-segment.hpp"

#include "core/dialogs.hpp"
#include "glue.hpp"
#include "io/sound.hpp"
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
void opeu_request_action2(uint8_t ds[65536], PendingEvent event, uint8_t side, uint16_t) {
  auto& data{ Data(ds) };
  data.arrival_unit = opeu_build_request_unit(ds, side);
  CallWords(ds, event, SpawnService::PREPARE, data.arrival_unit);
  if (data.side_status[side] != SIDE_PLAYED) return;
  CallWords(ds, event, SpawnService::CONTEXT, ARRIVAL_CONTEXT);
  auto const& unit{ Record<Unit>(data, data.arrival_unit) };
  CallWords(ds, event, SpawnService::LOCATION, unit.x, unit.y, side);
  CallWords(ds, event, SpawnService::VIEW, data.arrival_unit);
  data.arrival_side = side;
  CallWords(ds, event, SpawnService::SHOW, 0, side, ARRIVAL_PRESENT_OFFSET, ARRIVAL_PRESENT_SEGMENT);
  CallWords(ds, event, SpawnService::FINISH, 0);
}

uint16_t svc_reqspawn(uint8_t* ds, uint16_t id, uint16_t a, uint16_t b, uint16_t c, uint16_t d) {
  switch (static_cast<SpawnService>(id)) {
  case SpawnService::PREPARE: {
    opeu_arrival_prepare(ds, svc_maperase, a);
    break;
  }
  case SpawnService::CONTEXT:  opeu_set_turn_context(ds, svc_sound, (uint8_t)a); break;
  case SpawnService::LOCATION: {
    opeu_arrival_location(ds, svc_mappos, (uint8_t)a, (uint8_t)b, (uint8_t)c);
    break;
  }
  case SpawnService::VIEW: {
    opeu_arrival_view(ds, svc_mapunit, a);
    break;
  }
  case SpawnService::SHOW:   opeu_daily_show(ds, (const DialogHost*)port_dialogs_host(), a, (uint8_t)b, (uint32_t)d << 16 | c); break;
  case SpawnService::FINISH: opeu_finish_side_initialization(ds, svc_sound, (uint8_t)a); break;
  default:                   port_unhandled("reqspawn", id, 4);
  }
  return 0;
}
