// The arrival of reserve units (core/arrivals, core/reqspawn): the turn context it sets. Both show it with
// Presenter::ARRIVAL (common/presenter.hpp).
#pragma once

#include <cstdint>

namespace opeu::game::detail::arrival {

// The turn context (music and backdrop) of an arrival.
inline constexpr std::uint8_t ARRIVAL_CONTEXT{ 18 };
} // namespace opeu::game::detail::arrival

namespace opeu::game {
using detail::arrival::ARRIVAL_CONTEXT;
} // namespace opeu::game
