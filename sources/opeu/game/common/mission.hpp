// The special-forces missions: the kinds a side's mission order (Side::order) holds.
#pragma once

#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/msglook.hpp"

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

// The pictures a mission report shows and the far presenters (core/msnrept) the report runs.
inline constexpr std::uint16_t MISSION_PICTURE{ 0x120 }; // a mission succeeded
inline constexpr std::uint16_t FALLEN_PICTURE{ 0x12c };  // a general fell
inline constexpr std::uint16_t MISSION_CODE_SEGMENT{ 0x0e8e };
inline constexpr std::uint16_t FINISH_PRESENTER{ 0x0042 }; // opeu_mission_finish_present
inline constexpr std::uint16_t DELAY_PRESENTER{ 0x004a };  // opeu_mission_delay_present

// The near pointer of a mission message's text.
[[nodiscard]] inline auto MissionText(Game& game, std::uint16_t id) -> std::uint16_t {
  return opeu_message_lookup(game, static_cast<short>(id));
}

} // namespace opeu::game::detail::mission

namespace opeu::game {
using detail::mission::DELAY_PRESENTER;
using detail::mission::FALLEN_PICTURE;
using detail::mission::FINISH_PRESENTER;
using detail::mission::MISSION_CODE_SEGMENT;
using detail::mission::MISSION_PICTURE;
using detail::mission::MissionKind;
using detail::mission::MissionText;
} // namespace opeu::game
