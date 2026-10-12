// The six scenarios and the scenario number in the data segment's scenario byte.
#pragma once

#include "common/data-segment.hpp"

#include <cstdint>

namespace opeu::game::detail::scenario {

// The scenarios in menu order (data.scenario & SCENARIO_MASK).
enum class Scenario : std::uint8_t {
  FRANCE = 0, // France, 1940
  AFRICA = 1, // North Africa, 1942
  KURSK  = 2, // Kursk, 1943
  D_DAY  = 3, // Normandy, 1944
  BULGE  = 4, // the Ardennes, 1944-45
  BERLIN = 5, // Berlin, 1945
};

// The bits of the scenario byte that hold the scenario; the high bit is a flag the game keeps with it.
inline constexpr std::uint8_t SCENARIO_MASK{ 0x7f };

// The scenario being played.
[[nodiscard]] inline auto CurrentScenario(DataSegment const& data) noexcept -> Scenario {
  return static_cast<Scenario>(data.scenario & SCENARIO_MASK);
}

} // namespace opeu::game::detail::scenario

namespace opeu::game {
using detail::scenario::CurrentScenario;
using detail::scenario::Scenario;
using detail::scenario::SCENARIO_MASK;
} // namespace opeu::game
