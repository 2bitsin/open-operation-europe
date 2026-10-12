// The hourly countdown of the special-forces missions, and the first step of the hourly turn.
#include "core/msntick.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/mission.hpp"
#include "core/sideutil.hpp"
#include "data-segment.hpp"
#include "map/query.hpp"
#include "types.hpp"
#include "units/stats.hpp"

#include "core/msnkill.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

// Whether the mission's target still stands: a city the enemy holds, or a fielded enemy unit that is spotted.
uint16_t opeu_mission_target_valid(uint8_t ds[65536], uint16_t mission, uint8_t side) {
  auto const& order{ Record<Order>(Data(ds), mission) };
  switch (static_cast<MissionKind>(order.kind)) {
  case MissionKind::RAID_CITY:
  case MissionKind::SABOTAGE:
  case MissionKind::CUT_LINES_CITY:
  case MissionKind::RECON_CITY:     return opeu_objective_side(ds, order.target) == opeu_opposite_side(side);
  case MissionKind::RAID_UNIT:
  case MissionKind::CUT_LINES_UNIT:
  case MissionKind::ASSASSINATION:
  case MissionKind::RECON_UNIT:
    return (Record<Unit>(Data(ds), order.target).flags & Bits(UnitFlag::READY)) && opeu_unit_special(ds, order.target);
  default: return 0;
  }
}

// Each side's mission under way counts down an hour while its target stands and, at zero, runs; a mission whose
// target is gone is called off.
void opeu_mission_tick(uint8_t ds[65536], HostEvent event) {
  for (uint8_t side = 0; side < SIDE_COUNT; side++) {
    auto& mission{ Data(ds).sides[side].order };
    if (mission.kind == Bits(MissionKind::NONE)) continue;
    if (opeu_mission_target_valid(ds, Near(Data(ds), mission), side)) {
      if (--mission.progress == 0) {
        auto const& handler{ Data(ds).mission_handlers[mission.kind] };
        Call(ds, event, MISSION_HANDLER, handler.offset, handler.segment, side);
        mission.kind = Bits(MissionKind::NONE);
      }
    } else
      mission.kind = Bits(MissionKind::NONE);
  }
}

// The weather, the turn's preparation, the air operations and the special-forces missions.
void opeu_turn_hourly_a(uint8_t ds[65536], HostEvent event) {
  Call(ds, event, MISSION_WEATHER);
  Call(ds, event, MISSION_TURN_PREPARE);
  Call(ds, event, MISSION_AIR_OPERATIONS);
  opeu_mission_tick(ds, event);
}

uint16_t svc_msntick(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_mission("msntick", ds, id, args, count);
}
