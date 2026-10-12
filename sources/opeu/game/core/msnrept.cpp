// The special-forces missions' shared helpers: the officer's rating and experience, and the reports of a mission's
// success, its discovery by the enemy, its failure and a general's fall.
#include "core/msnrept.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/mission.hpp"
#include "common/side-color.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/personfl.hpp"
#include "core/sideutil.hpp"
#include "core/strings.hpp"
#include "core/template.hpp"
#include "data-segment.hpp"
#include "types.hpp"

#include <cstddef>
#include <utility>

#include "core/msnkill.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

// The message ids of the report formats.
enum ReportText : uint16_t {
  SUCCESS_FORMAT = 0,
  ENEMY_FORMAT   = 1,
  FAILURE_FORMAT = 2,
  ASSASSINATED   = 3, // " has been assassinated!" after a general's name
};

auto Text(uint8_t* ds, HostEvent event, uint16_t id) -> uint16_t {
  return Call(ds, event, MISSION_TEXT, id);
}

// The side's special-forces commander: the person template of that post for the army commander's nation.
auto SpecialForcesCommander(uint8_t* ds, uint8_t side) -> uint16_t {
  auto const& commander{ Record<Person>(
      Data(ds), opeu_find_side_record(ds, side, std::to_underlying(StaffRole::ARMY_COMMANDER))) };
  return opeu_find_person_template(ds, commander.status, std::to_underlying(StaffRole::SPECIAL_FORCES_COMMANDER));
}

// Formats a report into message: format, the side's colour and name, then text.
void FormatReport(uint8_t* ds, HostEvent event, uint16_t message, uint16_t format, uint8_t side, uint16_t text) {
  auto const format_text{ Text(ds, event, format) };
  Call(ds, event, MISSION_FORMAT, message, format_text, SideColorCode(side), Data(ds).side_names[side], text);
}

void ShowReport(uint8_t* ds, HostEvent event, uint16_t picture, uint8_t side, uint16_t presenter) {
  Call(ds, event, MISSION_DAILY_SHOW, picture, side, presenter, MISSION_CODE_SEGMENT);
}

} // namespace

// The officer's mean of leadership, charm, bravery and experience, at most his health.
uint16_t opeu_officer_rating(const uint8_t ds[65536], uint8_t side, uint8_t kind) {
  auto const& officer{ Record<Person>(Data(ds), opeu_find_side_record(ds, side, kind)) };
  return opeu_min16(static_cast<uint16_t>(officer.leadership + officer.charm + officer.bravery + officer.experience)
                        / 4,
                    officer.health);
}

// One point of experience for the officer, up to 99.
void opeu_officer_gain(uint8_t ds[65536], uint8_t side, uint8_t kind) {
  opeu_add_byte(1, 99, &Record<Person>(Data(ds), opeu_find_side_record(ds, side, kind)).experience);
}

void opeu_mission_finish_present(uint8_t ds[65536], HostEvent event) {
  Call(ds, event, MISSION_PENDING_FINISH, 3);
}

void opeu_mission_delay_present(uint8_t ds[65536], HostEvent event) {
  Call(ds, event, MISSION_DELAY_UNITS, 30);
}

// The special-forces commander tells his own side of the success and the map flashes the target's cell.
void opeu_mission_report_success(uint8_t ds[65536], HostEvent event, uint16_t frame, uint8_t side, uint16_t text,
                                 uint8_t x, uint8_t y) {
  auto const message{ Local(frame, 100) };
  auto const speaker{ SpecialForcesCommander(ds, side) };
  FormatReport(ds, event, message, SUCCESS_FORMAT, side, text);
  Call(ds, event, MISSION_MESSAGE, message, speaker);
  ShowReport(ds, event, MISSION_PICTURE, side, FINISH_PRESENTER);
  Call(ds, event, MISSION_FLASH, x, y, side);
}

// The attacked side's intelligence chief reports the enemy mission.
void opeu_mission_report_enemy(uint8_t ds[65536], HostEvent event, uint16_t frame, uint8_t side, uint16_t text,
                               uint8_t x, uint8_t y) {
  auto const message{ Local(frame, 100) };
  FormatReport(ds, event, message, ENEMY_FORMAT, side, text);
  auto const enemy{ opeu_opposite_side(side) };
  Call(ds, event, MISSION_MESSAGE, message,
       opeu_find_side_record(ds, enemy, std::to_underlying(StaffRole::INTELLIGENCE_CHIEF)));
  ShowReport(ds, event, MISSION_PICTURE, enemy, FINISH_PRESENTER);
  Call(ds, event, MISSION_FLASH, x, y, enemy);
}

// The special-forces commander reports the failure.
void opeu_mission_report_failure(uint8_t ds[65536], HostEvent event, uint16_t frame, uint8_t side, uint16_t text) {
  auto const message{ Local(frame, 100) };
  auto const speaker{ SpecialForcesCommander(ds, side) };
  FormatReport(ds, event, message, FAILURE_FORMAT, side, text);
  Call(ds, event, MISSION_MESSAGE, message, speaker);
}

// Writes the unit's name into destination, after "Enemy " when a player commands the side; returns destination.
uint16_t opeu_mission_target_name(uint8_t ds[65536], HostEvent event, uint16_t frame, uint8_t side, uint16_t unit,
                                  uint16_t destination) {
  auto const name{ Call(ds, event, MISSION_UNIT_NAME, unit, Local(frame, 32)) };
  auto const prefix{ Data(ds).side_status[side] == SIDE_PLAYED ? offsetof(DataSegment, mission_enemy_text)
                                                               : offsetof(DataSegment, mission_empty_text) };
  opeu_string_append(ds, opeu_string_copy(ds, destination, static_cast<uint16_t>(prefix)), name);
  return destination;
}

// A general was assassinated: a deputy takes his post, and a player's side hears of it from its intelligence chief.
void opeu_mission_person_killed(uint8_t ds[65536], HostEvent event, uint16_t frame, uint16_t person) {
  auto const message{ Local(frame, 50) };
  auto const side{ opeu_person_lacks_side0_flag(ds, person) };
  auto const played{ [ds, side] { return Data(ds).side_status[side] == SIDE_PLAYED; } };
  if (played()) {
    Call(ds, event, MISSION_TURN_CONTEXT, 0x10);
    Call(ds, event, MISSION_FINISH_SIDE, 0);
    opeu_string_copy(ds, message, person);
    opeu_string_append(ds, message, Text(ds, event, ASSASSINATED));
  }
  Call(ds, event, MISSION_DEPUTY, person);
  if (played()) {
    Call(ds, event, MISSION_MESSAGE, message,
         opeu_find_side_record(ds, side, std::to_underlying(StaffRole::INTELLIGENCE_CHIEF)));
    ShowReport(ds, event, FALLEN_PICTURE, side, DELAY_PRESENTER);
  }
}

uint16_t svc_msnrept(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_mission("msnrept", ds, id, args, count);
}

auto port_routine_opeu_mission_delay_present(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_mission_delay_present(ds, svc_msnrept);
  return 0;
}

auto port_routine_opeu_mission_finish_present(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_mission_finish_present(ds, svc_msnrept);
  return 0;
}
