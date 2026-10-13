// The special-forces assassination of a unit's generals, and the service table every mission routine calls through.
#include "core/msnkill.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/mission.hpp"
#include "core/msnrept.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/personfl.hpp"
#include "core/recindex.hpp"
#include "core/selunit.hpp"
#include "core/sideutil.hpp"
#include "core/strings.hpp"
#include "core/textout.hpp"
#include "data-segment.hpp"
#include "types.hpp"

#include <cstddef>
#include <utility>

#include "common/game.hpp"
#include "core/aircargo.hpp"
#include "core/deputy.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "core/weather.hpp"
#include "io/sound.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "map/mapunit.hpp"
#include "map/objdamag.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/casualty.hpp"
#include "units/unitname.hpp"

using namespace opeu::game;

namespace {

constexpr auto OFFICER{ std::to_underlying(StaffRole::OPERATIONS_CHIEF) };

// Message ids.
constexpr uint16_t ASSASSINATED{ 3 };          // " has been assassinated!"
constexpr uint16_t ASSASSINATION_MISSION{ 6 }; // " assassination mission"

auto Random(Game& game, HostEvent event, uint16_t bound) -> uint16_t {
  return Call(game, event, MISSION_RANDOM, bound);
}

auto General(Game& game, uint8_t side, StaffRole role) -> uint16_t {
  return opeu_find_side_record(game, side, std::to_underlying(role));
}

auto IsDeputy(Game& game, uint16_t person) -> bool {
  return Record<Person>(Data(game), person).role & Bits(PersonRole::DEPUTY);
}

} // namespace

// Against an enemy unit: the general of its division falls when the unit is the division's selected one; when it is
// the enemy side's selected unit, the intelligence and operations chiefs may fall (1 in 10 each) and the army
// commander be assassinated, a deputy taking his post (1 in 20). Any of these is a success, and every element of the
// unit then loses 30..49 morale.
void opeu_mission_assassinate(Game& game, HostEvent event, uint8_t side) {
  ServiceLocals const locals{ 0x50 };
  auto const          text{ locals.At(0) };
  auto const          target{ Data(game).sides[side].order.target };
  auto const          mission_text{ MissionText(game, event, ASSASSINATION_MISSION) };
  opeu_string_append(game, opeu_mission_target_name(game, event, side, target, text), mission_text);
  if (opeu_officer_rating(game, side, OFFICER) / 10 > Random(game, event, 100)) {
    auto       struck{ false };
    auto const enemy{ opeu_opposite_side(side) };
    if (opeu_find_group_selected_unit(game, opeu_unit_group(target)) == target) {
      auto const general{ opeu_find_group_record(game, enemy, opeu_unit_group_index(target)) };
      if (!IsDeputy(game, general)) {
        opeu_mission_person_killed(game, event, general);
        struck = true;
      }
    }
    if (opeu_find_side_selected_unit(game, enemy) == target) {
      for (auto const role : { StaffRole::INTELLIGENCE_CHIEF, StaffRole::OPERATIONS_CHIEF }) {
        auto const general{ General(game, enemy, role) };
        if (Random(game, event, 10) == 4 && !IsDeputy(game, general)) {
          opeu_mission_person_killed(game, event, general);
          struck = true;
        }
      }
      auto const commander{ General(game, enemy, StaffRole::ARMY_COMMANDER) };
      if (Random(game, event, 20) == 4) {
        Record<Person>(Data(game), commander).role |= Bits(PersonRole::DEPUTY);
        struck                                      = true;
        auto const commander_side{ opeu_person_lacks_side0_flag(game, commander) };
        Call(game, event, MISSION_TURN_CONTEXT, 0x10);
        Call(game, event, MISSION_FINISH_SIDE, 0);
        opeu_string_copy(game, text, commander);
        opeu_string_append(game, text, MissionText(game, event, ASSASSINATED));
        opeu_string_append(game, text, static_cast<uint16_t>(offsetof(DataSegment, mission_bold_off)));
        Call(game, event, MISSION_MESSAGE, text, General(game, commander_side, StaffRole::INTELLIGENCE_CHIEF));
        Call(game, event, MISSION_DAILY_SHOW, FALLEN_PICTURE, commander_side, DELAY_PRESENTER, MISSION_CODE_SEGMENT);
      }
    }
    if (struck) {
      auto& data{ Data(game) };
      auto& unit{ Record<Unit>(data, target) };
      if (data.side_status[side] == SIDE_PLAYED) opeu_mission_report_success(game, event, side, text, unit.x, unit.y);
      opeu_officer_gain(game, side, OFFICER);
      for (auto& element : Elements(data, unit)) opeu_sub_byte(Random(game, event, 20) + 30, &element.morale);
      return;
    }
  }
  opeu_mission_report_failure(game, event, side, text);
}

// The services of the mission routines (MissionService).
uint16_t svc_mission(const char* module, Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0, words[1];
  switch (id) {
  case MISSION_RANDOM:     r = port_rand_bounded(game, Arg(args, count, 0)); break;
  case MISSION_TEXT:       r = opeu_message_lookup(game, svc_msglook, (short)Arg(args, count, 0)); break;
  case MISSION_FORMAT:     opeu_request_format(game, { args, count }); break;
  case MISSION_MESSAGE:    opeu_control_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case MISSION_DAILY_SHOW: opeu_daily_show(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), (uint8_t)Arg(args, count, 1), Arg32(args, count, 2)); break;
  case MISSION_FLASH:      opeu_location_flash(game, svc_mapunit, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case MISSION_UNIT_NAME:  r = opeu_unit_name(game, svc_unitname, Arg(args, count, 0), Arg(args, count, 1)); break;
  case MISSION_OBJECTIVE_DAMAGE:
    opeu_objective_damage(game, svc_objdamag, Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), Arg(args, count, 5), Arg(args, count, 6));
    break;
  case MISSION_CASUALTIES:
    opeu_unit_casualties(game, svc_casualty, Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), Arg(args, count, 5));
    break;
  case MISSION_TURN_CONTEXT:   opeu_set_turn_context(game, svc_sound, (uint8_t)Arg(args, count, 0)); break;
  case MISSION_FINISH_SIDE:    opeu_finish_side_initialization(game, svc_sound, (uint8_t)Arg(args, count, 0)); break;
  case MISSION_DEPUTY:         opeu_initialize_deputy(game, Arg(args, count, 0)); break;
  case MISSION_PENDING_FINISH: opeu_pending_finish(game, svc_waitkey, (uint8_t)Arg(args, count, 0)); break;
  case MISSION_DELAY_UNITS:    opeu_delay_units(game, svc_waitkey, (uint8_t)Arg(args, count, 0)); break;
  case MISSION_WEATHER:        opeu_weather_update(game); break;
  case MISSION_TURN_PREPARE:   opeu_turn_prepare(game, (const DialogHost*)port_dialogs_host()); break;
  case MISSION_AIR_OPERATIONS: opeu_air_operations(game, svc_aircargo); break;
  case MISSION_HANDLER:
    words[0] = Arg(args, count, 2);
    port_call(game, (uint32_t)Arg(args, count, 1) * 16u + Arg(args, count, 0), words, 1);
    break;
  default: port_unhandled(module, id, count);
  }
  return r;
}

uint16_t svc_msnkill(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_mission("msnkill", game, id, args, count);
}

auto port_routine_opeu_mission_assassinate(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_mission_assassinate(game, svc_msnkill, (uint8_t)Arg(args, count, 0));
  return 0;
}
