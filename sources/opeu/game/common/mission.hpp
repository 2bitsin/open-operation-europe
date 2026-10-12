// The special-forces missions: the kinds a side's mission order (Side::order) holds and the services the mission
// routines (core/msn*) call through their event.
#pragma once

#include <cstdint>

namespace opeu::game::detail::mission {

// Side::order.kind of a special-forces mission; mission_handlers holds the routine of each kind.
enum class MissionKind : std::uint8_t {
  RAID_CITY      = 0,
  RAID_UNIT      = 1,
  SABOTAGE       = 2, // against a city
  CUT_LINES_CITY = 3, // cut the city's communications
  CUT_LINES_UNIT = 4, // cut the unit's communications
  ASSASSINATION  = 5, // against a unit's generals
  RECON_CITY     = 6,
  RECON_UNIT     = 7,
  NONE           = 0xff, // no mission under way
};

// The services a mission routine calls through its event (svc_mission in core/msnkill).
enum MissionService : std::uint16_t {
  MISSION_RANDOM           = 0,  // (bound) -> a number below bound
  MISSION_TEXT             = 1,  // (message id) -> the message's text
  MISSION_FORMAT           = 2,  // (destination, format, colour, side name, text)
  MISSION_MESSAGE          = 3,  // (text, person): a general's message
  MISSION_DAILY_SHOW       = 4,  // (delay, side, far presenter): show the side's report
  MISSION_FLASH            = 5,  // (x, y, side): flash the map cell
  MISSION_UNIT_NAME        = 6,  // (unit, buffer) -> buffer
  MISSION_OBJECTIVE_DAMAGE = 7,  // (objective, chance, base, supply, endurance, spread, heavy)
  MISSION_CASUALTIES       = 8,  // (unit, percent, kind, ...)
  MISSION_TURN_CONTEXT     = 9,  // (value)
  MISSION_FINISH_SIDE      = 10, // (value): finish the side's initialisation
  MISSION_DEPUTY           = 11, // (person): a deputy takes the fallen general's post
  MISSION_PENDING_FINISH   = 12, // (value)
  MISSION_DELAY_UNITS      = 13, // (value)
  MISSION_WEATHER          = 14, // the hourly weather change
  MISSION_TURN_PREPARE     = 15,
  MISSION_AIR_OPERATIONS   = 16,
  MISSION_HANDLER          = 17, // (offset, segment, side): run the mission's handler
};

// The pictures a mission report shows and the far presenters (core/msnrept) the report runs.
inline constexpr std::uint16_t MISSION_PICTURE{ 0x120 }; // a mission succeeded
inline constexpr std::uint16_t FALLEN_PICTURE{ 0x12c };  // a general fell
inline constexpr std::uint16_t MISSION_CODE_SEGMENT{ 0x0e8e };
inline constexpr std::uint16_t FINISH_PRESENTER{ 0x0042 }; // opeu_mission_finish_present
inline constexpr std::uint16_t DELAY_PRESENTER{ 0x004a }; // opeu_mission_delay_present

} // namespace opeu::game::detail::mission

namespace opeu::game {
using detail::mission::DELAY_PRESENTER;
using detail::mission::FALLEN_PICTURE;
using detail::mission::FINISH_PRESENTER;
using detail::mission::MISSION_AIR_OPERATIONS;
using detail::mission::MISSION_CASUALTIES;
using detail::mission::MISSION_CODE_SEGMENT;
using detail::mission::MISSION_DAILY_SHOW;
using detail::mission::MISSION_DELAY_UNITS;
using detail::mission::MISSION_DEPUTY;
using detail::mission::MISSION_FINISH_SIDE;
using detail::mission::MISSION_FLASH;
using detail::mission::MISSION_FORMAT;
using detail::mission::MISSION_HANDLER;
using detail::mission::MISSION_MESSAGE;
using detail::mission::MISSION_OBJECTIVE_DAMAGE;
using detail::mission::MISSION_PENDING_FINISH;
using detail::mission::MISSION_PICTURE;
using detail::mission::MISSION_RANDOM;
using detail::mission::MISSION_TEXT;
using detail::mission::MISSION_TURN_CONTEXT;
using detail::mission::MISSION_TURN_PREPARE;
using detail::mission::MISSION_UNIT_NAME;
using detail::mission::MISSION_WEATHER;
using detail::mission::MissionKind;
using detail::mission::MissionService;
} // namespace opeu::game
