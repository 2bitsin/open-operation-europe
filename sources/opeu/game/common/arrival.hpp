// The arrival of reserve units (core/arrivals, core/reqspawn): the presentation the daily display runs and the turn
// context it sets.
#pragma once

#include <cstdint>

namespace opeu::game::detail::arrival {

// The far address of opeu_arrival_present (core/arrshow), which the daily display calls back.
inline constexpr std::uint16_t ARRIVAL_PRESENT_OFFSET{ 0x816 };
inline constexpr std::uint16_t ARRIVAL_PRESENT_SEGMENT{ 0x1270 };

// The turn context (music and backdrop) of an arrival.
inline constexpr std::uint8_t ARRIVAL_CONTEXT{ 18 };
} // namespace opeu::game::detail::arrival

namespace opeu::game {
using detail::arrival::ARRIVAL_CONTEXT;
using detail::arrival::ARRIVAL_PRESENT_OFFSET;
using detail::arrival::ARRIVAL_PRESENT_SEGMENT;
} // namespace opeu::game
