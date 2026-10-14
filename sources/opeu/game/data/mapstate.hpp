// The initial values of the data segment's map working arrays: turn order, objective and unit links, map cells and
// influence map. All zero.
#pragma once

#include <array>
#include <cstdint>

namespace opeu::game::detail::mapstate {

inline constexpr std::array<std::uint8_t, 64> INITIAL_TURN_ORDER{};

inline constexpr std::array<std::uint16_t, 42> INITIAL_OBJECTIVE_LINKS{};

inline constexpr std::array<std::uint16_t, 8> INITIAL_UNIT_LINKS{};

inline constexpr std::array<std::uint8_t, 1600> INITIAL_MAP_CELLS{};

inline constexpr std::array<std::array<std::int16_t, 10>, 10> INITIAL_INFLUENCE_MAP{};

} // namespace opeu::game::detail::mapstate

namespace opeu::game {
using detail::mapstate::INITIAL_INFLUENCE_MAP;
using detail::mapstate::INITIAL_MAP_CELLS;
using detail::mapstate::INITIAL_OBJECTIVE_LINKS;
using detail::mapstate::INITIAL_TURN_ORDER;
using detail::mapstate::INITIAL_UNIT_LINKS;
} // namespace opeu::game
