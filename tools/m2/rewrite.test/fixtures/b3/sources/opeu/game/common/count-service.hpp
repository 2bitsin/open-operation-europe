// The services of the count: an enumeration and its re-export, nothing else.
#pragma once

#include <cstdint>

namespace opeu::game::detail::count_service {
enum class CountService : std::uint16_t {
  TALLY = 0,
};
} // namespace opeu::game::detail::count_service

namespace opeu::game {
using detail::count_service::CountService;
} // namespace opeu::game
