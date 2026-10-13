// The fixture's word-packing wrapper nothing calls any more, beside a constant still named.
#pragma once

#include "common/events.hpp"

#include <cstdint>

namespace opeu::game::detail::list_call {

// A call with its argument words.
template <typename... Words>
inline auto List(Game& game, HostEvent event, std::uint16_t call, Words... words) -> std::uint16_t {
  std::uint16_t args[]{ static_cast<std::uint16_t>(words)..., 0 };
  return event(game, call, args, sizeof...(Words));
}

// The rows a list shows.
inline constexpr std::uint8_t ROWS{ 8 };

} // namespace opeu::game::detail::list_call

namespace opeu::game {
using detail::list_call::List;
using detail::list_call::ROWS;
} // namespace opeu::game
