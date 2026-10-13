// The hourly countdown of the special-forces missions, and the first step of the hourly turn.
#include "core/msntick.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/mission.hpp"
#include "core/aircargo.hpp"
#include "core/dialogs.hpp"
#include "core/sideutil.hpp"
#include "core/weather.hpp"
#include "data-segment.hpp"
#include "map/query.hpp"
#include "types.hpp"
#include "units/stats.hpp"

#include <array>

#include "common/game.hpp"
#include "core/msnkill.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

// Whether the mission's target still stands: a city the enemy holds, or a fielded enemy unit that is spotted.
uint16_t opeu_mission_target_valid(Game& game, uint16_t mission, uint8_t side) {
  auto const& order{ Record<Order>(Data(game), mission) };
  switch (static_cast<MissionKind>(order.kind)) {
  case MissionKind::RAID_CITY:
  case MissionKind::SABOTAGE:
  case MissionKind::CUT_LINES_CITY:
  case MissionKind::RECON_CITY:     return opeu_objective_side(game, order.target) == opeu_opposite_side(side);
  case MissionKind::RAID_UNIT:
  case MissionKind::CUT_LINES_UNIT:
  case MissionKind::ASSASSINATION:
  case MissionKind::RECON_UNIT:     return (Record<Unit>(Data(game), order.target).flags & Bits(UnitFlag::READY)) && opeu_unit_special(game, order.target);
  default:                          return 0;
  }
}

// Each side's mission under way counts down an hour while its target stands and, at zero, runs; a mission whose
// target is gone is called off.
void opeu_mission_tick(Game& game) {
  for (uint8_t side = 0; side < SIDE_COUNT; side++) {
    auto& mission{ Data(game).sides[side].order };
    if (mission.kind == Bits(MissionKind::NONE)) continue;
    if (opeu_mission_target_valid(game, Near(Data(game), mission), side)) {
      if (--mission.progress == 0) {
        auto const&                   handler{ Data(game).mission_handlers[mission.kind] };
        std::array<uint16_t, 1> const words{ side };
        port_call(game, static_cast<uint32_t>(handler.segment) * 16u + handler.offset, words.data(), words.size());
        mission.kind = Bits(MissionKind::NONE);
      }
    } else
      mission.kind = Bits(MissionKind::NONE);
  }
}

// The weather, the turn's preparation, the air operations and the special-forces missions.
void opeu_turn_hourly_a(Game& game) {
  opeu_weather_update(game);
  opeu_turn_prepare(game, static_cast<const DialogHost*>(port_dialogs_host()));
  opeu_air_operations(game);
  opeu_mission_tick(game);
}
