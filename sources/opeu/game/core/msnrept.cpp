// The special-forces missions' shared helpers: the officer's rating and experience, and the reports of a mission's
// success, its discovery by the enemy, its failure and a general's fall.
#include "core/msnrept.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/mission.hpp"
#include "common/side-color.hpp"
#include "core/deputy.hpp"
#include "core/dialogs.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/personfl.hpp"
#include "core/sideutil.hpp"
#include "core/strings.hpp"
#include "core/template.hpp"
#include "core/textout.hpp"
#include "data-segment.hpp"
#include "io/sound.hpp"
#include "io/waitkey.hpp"
#include "map/mapunit.hpp"
#include "types.hpp"
#include "units/unitname.hpp"

#include <array>
#include <cstddef>
#include <utility>

#include "common/game.hpp"
#include "core/msnkill.hpp"
#include "link/glue.hpp"
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

// The side's special-forces commander: the person template of that post for the army commander's nation.
auto SpecialForcesCommander(Game& game, uint8_t side) -> uint16_t {
  auto const& commander{ Record<Person>(Data(game), opeu_find_side_record(game, side, std::to_underlying(StaffRole::ARMY_COMMANDER))) };
  return opeu_find_person_template(game, commander.status, std::to_underlying(StaffRole::SPECIAL_FORCES_COMMANDER));
}

// Formats a report into message: format, the side's colour and name, then text.
void FormatReport(Game& game, uint16_t message, uint16_t format, uint8_t side, uint16_t text) {
  auto const format_text{ MissionText(game, format) };
  opeu_request_format(game, std::array<std::uint16_t, 5>{ static_cast<std::uint16_t>(message), static_cast<std::uint16_t>(format_text), static_cast<std::uint16_t>(SideColorCode(side)),
                                                          static_cast<std::uint16_t>(Data(game).side_names[side]), static_cast<std::uint16_t>(text) });
}

void ShowReport(Game& game, uint16_t picture, uint8_t side, uint16_t presenter) {
  opeu_daily_show(game, static_cast<const DialogHost*>(port_dialogs_host()), picture, static_cast<uint8_t>(side),
                  (static_cast<std::uint32_t>(static_cast<std::uint16_t>(presenter)) | static_cast<std::uint32_t>(static_cast<std::uint16_t>(MISSION_CODE_SEGMENT)) << 16));
}

} // namespace

// The officer's mean of leadership, charm, bravery and experience, at most his health.
uint16_t opeu_officer_rating(Game const& game, uint8_t side, uint8_t kind) {
  auto const& officer{ Record<Person>(Data(game), opeu_find_side_record(game, side, kind)) };
  return opeu_min16(static_cast<uint16_t>(officer.leadership + officer.charm + officer.bravery + officer.experience) / 4, officer.health);
}

// One point of experience for the officer, up to 99.
void opeu_officer_gain(Game& game, uint8_t side, uint8_t kind) {
  opeu_add_byte(1, 99, &Record<Person>(Data(game), opeu_find_side_record(game, side, kind)).experience);
}

void opeu_mission_finish_present(Game& game) {
  opeu_pending_finish(game, static_cast<uint8_t>(3));
}

void opeu_mission_delay_present(Game& game) {
  opeu_delay_units(game, static_cast<uint8_t>(30));
}

// The special-forces commander tells his own side of the success and the map flashes the target's cell.
void opeu_mission_report_success(Game& game, uint8_t side, uint16_t text, uint8_t x, uint8_t y) {
  ServiceLocals const locals{ 0x64 };
  auto const          message{ locals.At(0) };
  auto const          speaker{ SpecialForcesCommander(game, side) };
  FormatReport(game, message, SUCCESS_FORMAT, side, text);
  opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), message, speaker);
  ShowReport(game, MISSION_PICTURE, side, FINISH_PRESENTER);
  opeu_location_flash(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y), static_cast<uint8_t>(side));
}

// The attacked side's intelligence chief reports the enemy mission.
void opeu_mission_report_enemy(Game& game, uint8_t side, uint16_t text, uint8_t x, uint8_t y) {
  ServiceLocals const locals{ 0x64 };
  auto const          message{ locals.At(0) };
  FormatReport(game, message, ENEMY_FORMAT, side, text);
  auto const enemy{ opeu_opposite_side(side) };
  opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), message, opeu_find_side_record(game, enemy, std::to_underlying(StaffRole::INTELLIGENCE_CHIEF)));
  ShowReport(game, MISSION_PICTURE, enemy, FINISH_PRESENTER);
  opeu_location_flash(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y), static_cast<uint8_t>(enemy));
}

// The special-forces commander reports the failure.
void opeu_mission_report_failure(Game& game, uint8_t side, uint16_t text) {
  ServiceLocals const locals{ 0x64 };
  auto const          message{ locals.At(0) };
  auto const          speaker{ SpecialForcesCommander(game, side) };
  FormatReport(game, message, FAILURE_FORMAT, side, text);
  opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), message, speaker);
}

// Writes the unit's name into destination, after "Enemy " when a player commands the side; returns destination.
uint16_t opeu_mission_target_name(Game& game, uint8_t side, uint16_t unit, uint16_t destination) {
  ServiceLocals const locals{ 32 }; // the name the unit-name service writes
  auto const          name{ opeu_unit_name(game, unit, locals.At(0)) };
  auto const          prefix{ Data(game).side_status[side] == SIDE_PLAYED ? offsetof(DataSegment, mission_enemy_text) : offsetof(DataSegment, mission_empty_text) };
  opeu_string_append(game, opeu_string_copy(game, destination, static_cast<uint16_t>(prefix)), name);
  return destination;
}

// A general was assassinated: a deputy takes his post, and a player's side hears of it from its intelligence chief.
void opeu_mission_person_killed(Game& game, uint16_t person) {
  ServiceLocals const locals{ 0x32 };
  auto const          message{ locals.At(0) };
  auto const          side{ opeu_person_lacks_side0_flag(game, person) };
  auto const          played{ [game, side] { return Data(game).side_status[side] == SIDE_PLAYED; } };
  if (played()) {
    opeu_set_turn_context(game, svc_sound, static_cast<uint8_t>(0x10));
    opeu_finish_side_initialization(game, svc_sound, static_cast<uint8_t>(0));
    opeu_string_copy(game, message, person);
    opeu_string_append(game, message, MissionText(game, ASSASSINATED));
  }
  opeu_initialize_deputy(game, person);
  if (played()) {
    opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), message, opeu_find_side_record(game, side, std::to_underlying(StaffRole::INTELLIGENCE_CHIEF)));
    ShowReport(game, FALLEN_PICTURE, side, DELAY_PRESENTER);
  }
}

auto port_routine_opeu_mission_delay_present(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_mission_delay_present(game);
  return 0;
}

auto port_routine_opeu_mission_finish_present(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_mission_finish_present(game);
  return 0;
}
