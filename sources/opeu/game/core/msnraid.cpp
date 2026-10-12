// The special-forces raids, sabotage and cuts of communications: each rolls against the operations chief's rating,
// reports the outcome and, on success, damages the target.
#include "types.hpp"
#include "core/msnraid.hpp"
#include "core/msnrept.hpp"
#include "core/strings.hpp"
#include "types/events.hpp"
#include "common/memory.hpp"
#include "common/mission.hpp"
#include "data-segment.hpp"

#include <cstddef>
#include <utility>

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"
#include "core/msnkill.hpp"

using namespace opeu::game;

namespace {

constexpr auto OFFICER{ std::to_underlying(StaffRole::OPERATIONS_CHIEF) };

// Message ids of the cut-communications reports.
constexpr uint16_t CUT_LINES_TEXT{ 0x6a }; // "mission to cut comm of "
constexpr uint16_t LINES_CUT_TAIL{ 0x65 }; // " had their communications cut!"

constexpr auto At(std::size_t offset) -> uint16_t {
  return static_cast<uint16_t>(offset);
}

auto Target(uint8_t* ds, uint8_t side) -> uint16_t {
  return Data(ds).sides[side].order.target;
}

auto Played(uint8_t* ds, uint8_t side) -> bool {
  return Data(ds).side_status[side] == SIDE_PLAYED;
}

// Whether the roll succeeds: the officer's rating divided by divisor against 0..99.
auto Succeeds(uint8_t* ds, HostEvent event, uint8_t side, uint16_t divisor)
    -> bool {
  return opeu_officer_rating(ds, side, OFFICER) / divisor >
         Call(ds, event, MISSION_RANDOM, 100);
}

auto Text(uint8_t* ds, HostEvent event, uint16_t id) -> uint16_t {
  return Call(ds, event, MISSION_TEXT, id);
}

} // namespace

void opeu_mission_raid_objective(uint8_t ds[65536], HostEvent event,
                                 uint16_t frame, uint8_t side) {
  auto const  text{ Local(frame, 50) };
  auto const  target{ Target(ds, side) };
  auto const& city{ Record<Objective>(Data(ds), target) };
  opeu_string_copy(ds, text, At(offsetof(DataSegment, raid_attempt_text)));
  opeu_string_append(ds, text, target);
  if (!Succeeds(ds, event, side, 4)) {
    opeu_mission_report_failure(ds, event, frame, side, text);
    return;
  }
  if (Played(ds, side))
    opeu_mission_report_success(ds, event, frame, side, text, city.x, city.y);
  else {
    opeu_string_append(ds, opeu_string_copy(ds, text, target),
                       At(offsetof(DataSegment, raid_attempt_tail)));
    opeu_mission_report_enemy(ds, event, frame, side, text, city.x, city.y);
  }
  Call(ds, event, MISSION_OBJECTIVE_DAMAGE, target, 30, 3, 1000, 0, 15, 1);
  opeu_officer_gain(ds, side, OFFICER);
}

void opeu_mission_raid_unit(uint8_t ds[65536], HostEvent event, uint16_t frame,
                            uint8_t side) {
  auto const  name{ Local(frame, 12) };
  auto const  text{ Local(frame, 62) };
  auto const  target{ Target(ds, side) };
  auto const& unit{ Record<Unit>(Data(ds), target) };
  opeu_string_copy(ds, text, At(offsetof(DataSegment, raid_unit_text)));
  opeu_string_append(ds, text,
                     Call(ds, event, MISSION_UNIT_NAME, target, name));
  if (!Succeeds(ds, event, side, 4)) {
    opeu_mission_report_failure(ds, event, frame, side, text);
    return;
  }
  if (Played(ds, side))
    opeu_mission_report_success(ds, event, frame, side, text, unit.x, unit.y);
  else {
    opeu_string_append(
        ds, opeu_mission_target_name(ds, event, frame, side, target, text),
        At(offsetof(DataSegment, raid_unit_tail)));
    opeu_mission_report_enemy(ds, event, frame, side, text, unit.x, unit.y);
  }
  Call(ds, event, MISSION_CASUALTIES, target, 30, 3, 15, 1, 1);
  opeu_officer_gain(ds, side, OFFICER);
}

void opeu_mission_sabotage(uint8_t ds[65536], HostEvent event, uint16_t frame,
                           uint8_t side) {
  auto const  text{ Local(frame, 50) };
  auto const  target{ Target(ds, side) };
  auto const& city{ Record<Objective>(Data(ds), target) };
  opeu_string_copy(ds, text, At(offsetof(DataSegment, sabotage_text)));
  opeu_string_append(ds, text, target);
  if (!Succeeds(ds, event, side, 3)) {
    opeu_mission_report_failure(ds, event, frame, side, text);
    return;
  }
  if (Played(ds, side))
    opeu_mission_report_success(ds, event, frame, side, text, city.x, city.y);
  else {
    opeu_string_append(ds, opeu_string_copy(ds, text, target),
                       At(offsetof(DataSegment, sabotage_tail)));
    opeu_mission_report_enemy(ds, event, frame, side, text, city.x, city.y);
  }
  Call(ds, event, MISSION_OBJECTIVE_DAMAGE, target, 10, 2, 5000, 5, 5, 0);
  opeu_officer_gain(ds, side, OFFICER);
}

// Cuts a city's communications.
void opeu_mission_objective_3(uint8_t ds[65536], HostEvent event,
                              uint16_t frame, uint8_t side) {
  auto const text{ Local(frame, 120) };
  auto const target{ Target(ds, side) };
  auto&      city{ Record<Objective>(Data(ds), target) };
  opeu_string_copy(ds, text, Text(ds, event, CUT_LINES_TEXT));
  opeu_string_append(ds, text, target);
  opeu_string_append(ds, text, At(offsetof(DataSegment, mission3_suffix)));
  if (!Succeeds(ds, event, side, 2)) {
    opeu_mission_report_failure(ds, event, frame, side, text);
    return;
  }
  if (Played(ds, side))
    opeu_mission_report_success(ds, event, frame, side, text, city.x, city.y);
  else {
    opeu_string_copy(ds, text, target);
    opeu_string_append(ds, text, Text(ds, event, LINES_CUT_TAIL));
    opeu_string_append(ds, text, At(offsetof(DataSegment, mission3_tail)));
    opeu_mission_report_enemy(ds, event, frame, side, text, city.x, city.y);
  }
  city.flags &= ~Bits(ObjectiveFlag::COMMUNICATIONS);
  opeu_officer_gain(ds, side, OFFICER);
}

// Cuts a unit's communications.
void opeu_mission_unit_4(uint8_t ds[65536], HostEvent event, uint16_t frame,
                         uint8_t side) {
  auto const text{ Local(frame, 80) };
  auto const name{ Local(frame, 110) };
  auto const target{ Target(ds, side) };
  auto&      data{ Data(ds) };
  auto&      unit{ Record<Unit>(data, target) };
  opeu_string_copy(ds, text, Text(ds, event, CUT_LINES_TEXT));
  opeu_string_append(ds, text,
                     Call(ds, event, MISSION_UNIT_NAME, target, name));
  if (!Succeeds(ds, event, side, 2)) {
    opeu_mission_report_failure(ds, event, frame, side, text);
    return;
  }
  if (Played(ds, side))
    opeu_mission_report_success(ds, event, frame, side, text, unit.x, unit.y);
  else {
    auto const tail{ Text(ds, event, LINES_CUT_TAIL) };
    opeu_string_append(
        ds, opeu_mission_target_name(ds, event, frame, side, target, text),
        tail);
    opeu_mission_report_enemy(ds, event, frame, side, text, unit.x, unit.y);
  }
  // The views redraw.
  data.view_side = data.overview_side = 0xff;
  unit.flags &= ~Bits(UnitFlag::COMMUNICATIONS);
  opeu_officer_gain(ds, side, OFFICER);
}

uint16_t svc_msnraid(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count) {
  return svc_mission("msnraid", ds, id, args, count);
}

auto port_routine_opeu_mission_objective_3(std::uint8_t*        ds,
                                           std::uint16_t const* args,
                                           unsigned count) -> std::uint32_t {
  uint16_t frame = port_frame_enter();
  opeu_mission_objective_3(ds, svc_msnraid, frame,
                           (uint8_t)Arg(args, count, 0));
  port_frame_leave();
  return 0;
}

auto port_routine_opeu_mission_raid_objective(std::uint8_t*        ds,
                                              std::uint16_t const* args,
                                              unsigned count) -> std::uint32_t {
  uint16_t frame = port_frame_enter();
  opeu_mission_raid_objective(ds, svc_msnraid, frame,
                              (uint8_t)Arg(args, count, 0));
  port_frame_leave();
  return 0;
}

auto port_routine_opeu_mission_raid_unit(std::uint8_t*        ds,
                                         std::uint16_t const* args,
                                         unsigned count) -> std::uint32_t {
  uint16_t frame = port_frame_enter();
  opeu_mission_raid_unit(ds, svc_msnraid, frame, (uint8_t)Arg(args, count, 0));
  port_frame_leave();
  return 0;
}

auto port_routine_opeu_mission_sabotage(std::uint8_t*        ds,
                                        std::uint16_t const* args,
                                        unsigned count) -> std::uint32_t {
  uint16_t frame = port_frame_enter();
  opeu_mission_sabotage(ds, svc_msnraid, frame, (uint8_t)Arg(args, count, 0));
  port_frame_leave();
  return 0;
}

auto port_routine_opeu_mission_unit_4(std::uint8_t*        ds,
                                      std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  uint16_t frame = port_frame_enter();
  opeu_mission_unit_4(ds, svc_msnraid, frame, (uint8_t)Arg(args, count, 0));
  port_frame_leave();
  return 0;
}
