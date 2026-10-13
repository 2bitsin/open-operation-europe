// What the air flights (core/aircargo, airstrk, airshow, evbomb) share: the side records and person template that
// speak for them and the screen rows a plane flies between.
#pragma once

#include <cstdint>

namespace opeu::game::detail::air_flight {

// The side record kind of the air officer: his ratings and gains decide a flight, and he reports it.
inline constexpr std::uint8_t AIR_OFFICER_KIND{ 3 };
// The side record kind of the commander whose air-officer template presents a flight.
inline constexpr std::uint8_t FLIGHT_COMMANDER_KIND{ 1 };
// The person template of the air officer.
inline constexpr std::uint8_t AIR_OFFICER_TEMPLATE{ 6 };

// A plane flies up the screen from FLIGHT_TOP_ROW to FLIGHT_BOTTOM_ROW.
inline constexpr std::uint16_t FLIGHT_TOP_ROW{ 0x15c };
inline constexpr std::uint16_t FLIGHT_BOTTOM_ROW{ 0x54 };

} // namespace opeu::game::detail::air_flight

namespace opeu::game {
using detail::air_flight::AIR_OFFICER_KIND;
using detail::air_flight::AIR_OFFICER_TEMPLATE;
using detail::air_flight::FLIGHT_BOTTOM_ROW;
using detail::air_flight::FLIGHT_COMMANDER_KIND;
using detail::air_flight::FLIGHT_TOP_ROW;
} // namespace opeu::game
