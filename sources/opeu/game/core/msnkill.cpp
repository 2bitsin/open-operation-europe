// The special-forces assassination of a unit's generals, and the service table every mission routine calls through.
#include "types.hpp"
#include "core/msnkill.hpp"
#include "core/msnrept.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/personfl.hpp"
#include "core/recindex.hpp"
#include "core/selunit.hpp"
#include "core/sideutil.hpp"
#include "core/strings.hpp"
#include "types/events.hpp"
#include "common/element-chain.hpp"
#include "common/memory.hpp"
#include "common/mission.hpp"
#include "data-segment.hpp"

#include <cstddef>
#include <utility>

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"
#include "core/aircargo.hpp"
#include "core/deputy.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "core/weather.hpp"
#include "io/sound.hpp"
#include "io/waitkey.hpp"
#include "map/mapunit.hpp"
#include "map/objdamag.hpp"
#include "units/casualty.hpp"
#include "units/unitname.hpp"

using namespace opeu::game;

namespace {

constexpr auto OFFICER{ std::to_underlying(StaffRole::OPERATIONS_CHIEF) };

// Message ids.
constexpr uint16_t ASSASSINATED{ 3 };          // " has been assassinated!"
constexpr uint16_t ASSASSINATION_MISSION{ 6 }; // " assassination mission"

auto Random(uint8_t* ds, HostEvent event, uint16_t bound) -> uint16_t {
  return Call(ds, event, MISSION_RANDOM, bound);
}

auto Text(uint8_t* ds, HostEvent event, uint16_t id) -> uint16_t {
  return Call(ds, event, MISSION_TEXT, id);
}

auto General(uint8_t* ds, uint8_t side, StaffRole role) -> uint16_t {
  return opeu_find_side_record(ds, side, std::to_underlying(role));
}

auto IsDeputy(uint8_t* ds, uint16_t person) -> bool {
  return Record<Person>(Data(ds), person).role & Bits(PersonRole::DEPUTY);
}

} // namespace

// Against an enemy unit: the general of its division falls when the unit is the division's selected one; when it is
// the enemy side's selected unit, the intelligence and operations chiefs may fall (1 in 10 each) and the army
// commander be assassinated, a deputy taking his post (1 in 20). Any of these is a success, and every element of the
// unit then loses 30..49 morale.
void opeu_mission_assassinate(uint8_t ds[65536], HostEvent event,
                              uint16_t frame, uint8_t side) {
  auto const text{ Local(frame, 80) };
  auto const target{ Data(ds).sides[side].order.target };
  auto const mission_text{ Text(ds, event, ASSASSINATION_MISSION) };
  opeu_string_append(
      ds, opeu_mission_target_name(ds, event, frame, side, target, text),
      mission_text);
  if (opeu_officer_rating(ds, side, OFFICER) / 10 > Random(ds, event, 100)) {
    auto       struck{ false };
    auto const enemy{ opeu_opposite_side(side) };
    if (opeu_find_group_selected_unit(ds, opeu_unit_group(target)) == target) {
      auto const general{ opeu_find_group_record(
          ds, enemy, opeu_unit_group_index(target)) };
      if (!IsDeputy(ds, general)) {
        opeu_mission_person_killed(ds, event, frame, general);
        struck = true;
      }
    }
    if (opeu_find_side_selected_unit(ds, enemy) == target) {
      for (auto const role :
           { StaffRole::INTELLIGENCE_CHIEF, StaffRole::OPERATIONS_CHIEF }) {
        auto const general{ General(ds, enemy, role) };
        if (Random(ds, event, 10) == 4 && !IsDeputy(ds, general)) {
          opeu_mission_person_killed(ds, event, frame, general);
          struck = true;
        }
      }
      auto const commander{ General(ds, enemy, StaffRole::ARMY_COMMANDER) };
      if (Random(ds, event, 20) == 4) {
        Record<Person>(Data(ds), commander).role |= Bits(PersonRole::DEPUTY);
        struck = true;
        auto const commander_side{ opeu_person_lacks_side0_flag(ds,
                                                                commander) };
        Call(ds, event, MISSION_TURN_CONTEXT, 0x10);
        Call(ds, event, MISSION_FINISH_SIDE, 0);
        opeu_string_copy(ds, text, commander);
        opeu_string_append(ds, text, Text(ds, event, ASSASSINATED));
        opeu_string_append(
            ds, text,
            static_cast<uint16_t>(offsetof(DataSegment, mission_bold_off)));
        Call(ds, event, MISSION_MESSAGE, text,
             General(ds, commander_side, StaffRole::INTELLIGENCE_CHIEF));
        Call(ds, event, MISSION_DAILY_SHOW, FALLEN_PICTURE, commander_side,
             DELAY_PRESENTER, MISSION_CODE_SEGMENT);
      }
    }
    if (struck) {
      auto& data{ Data(ds) };
      auto& unit{ Record<Unit>(data, target) };
      if (data.side_status[side] == SIDE_PLAYED)
        opeu_mission_report_success(ds, event, frame, side, text, unit.x,
                                    unit.y);
      opeu_officer_gain(ds, side, OFFICER);
      for (auto& element : Elements(data, unit))
        opeu_sub_byte(Random(ds, event, 20) + 30, &element.morale);
      return;
    }
  }
  opeu_mission_report_failure(ds, event, frame, side, text);
}

// The services of the mission routines (MissionService).
uint16_t svc_mission(const char* module, uint8_t* ds, uint16_t id,
                     const uint16_t* args, uint16_t count) {
  uint16_t r = 0, frame, sp, words[1];
  switch (id) {
    case MISSION_RANDOM: r = port_rand_bounded(ds, Arg(args, count, 0)); break;
    case MISSION_TEXT:
      r = opeu_message_lookup(ds, svc_msglook, (short)Arg(args, count, 0));
      break;
    case MISSION_FORMAT: c2_request_format(ds, args, count); break;
    case MISSION_MESSAGE:
      opeu_control_message(ds, (const DialogHost*)port_dialogs_host(),
                           Arg(args, count, 0), Arg(args, count, 1));
      break;
    case MISSION_DAILY_SHOW:
      opeu_daily_show(ds, (const DialogHost*)port_dialogs_host(),
                      Arg(args, count, 0), (uint8_t)Arg(args, count, 1),
                      Arg32(args, count, 2));
      break;
    case MISSION_FLASH:
      frame = port_frame_enter();
      opeu_location_flash(ds, svc_mapunit, frame, (uint8_t)Arg(args, count, 0),
                          (uint8_t)Arg(args, count, 1),
                          (uint8_t)Arg(args, count, 2));
      port_frame_leave();
      break;
    case MISSION_UNIT_NAME:
      r = opeu_unit_name(ds, svc_unitname, Arg(args, count, 0),
                         Arg(args, count, 1));
      break;
    case MISSION_OBJECTIVE_DAMAGE:
      frame = port_frame_enter();
      opeu_objective_damage(ds, svc_objdamag, frame, Arg(args, count, 0),
                            Arg(args, count, 1), Arg(args, count, 2),
                            Arg(args, count, 3), Arg(args, count, 4),
                            Arg(args, count, 5), Arg(args, count, 6));
      port_frame_leave();
      break;
    case MISSION_CASUALTIES:
      frame = port_frame_enter();
      opeu_unit_casualties(ds, svc_casualty, frame, Arg(args, count, 0),
                           Arg(args, count, 1), Arg(args, count, 2),
                           Arg(args, count, 3), Arg(args, count, 4),
                           Arg(args, count, 5));
      port_frame_leave();
      break;
    case MISSION_TURN_CONTEXT:
      opeu_set_turn_context(ds, svc_sound, (uint8_t)Arg(args, count, 0));
      break;
    case MISSION_FINISH_SIDE:
      opeu_finish_side_initialization(ds, svc_sound,
                                      (uint8_t)Arg(args, count, 0));
      break;
    case MISSION_DEPUTY: opeu_initialize_deputy(ds, Arg(args, count, 0)); break;
    case MISSION_PENDING_FINISH:
      opeu_pending_finish(ds, svc_waitkey, (uint8_t)Arg(args, count, 0));
      break;
    case MISSION_DELAY_UNITS:
      opeu_delay_units(ds, svc_waitkey, (uint8_t)Arg(args, count, 0));
      break;
    case MISSION_WEATHER: opeu_weather_update(ds); break;
    case MISSION_TURN_PREPARE:
      opeu_turn_prepare(ds, (const DialogHost*)port_dialogs_host());
      break;
    case MISSION_AIR_OPERATIONS:
      sp = port_stack_enter(PORT_FRAME);
      opeu_air_operations(ds, svc_aircargo, sp);
      port_stack_leave(PORT_FRAME);
      break;
    case MISSION_HANDLER:
      words[0] = Arg(args, count, 2);
      port_call(ds, (uint32_t)Arg(args, count, 1) * 16u + Arg(args, count, 0),
                words, 1);
      break;
    default: port_unhandled(module, id, count);
  }
  return r;
}

uint16_t svc_msnkill(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count) {
  return svc_mission("msnkill", ds, id, args, count);
}

auto port_routine_opeu_mission_assassinate(std::uint8_t*        ds,
                                           std::uint16_t const* args,
                                           unsigned count) -> std::uint32_t {
  uint16_t frame = port_frame_enter();
  opeu_mission_assassinate(ds, svc_msnkill, frame,
                           (uint8_t)Arg(args, count, 0));
  port_frame_leave();
  return 0;
}
