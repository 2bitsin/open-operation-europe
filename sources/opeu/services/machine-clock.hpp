// What the game reads the time from: the BIOS tick counter (18.2 Hz since midnight; headless, since the run began).
#pragma once

#include <cstdint>

namespace opeu::services::detail::machine_clock {

class Clock {
public:
  Clock()                                = default;
  Clock(Clock const&)                    = delete;
  Clock(Clock&&)                         = delete;
  auto operator=(Clock const&) -> Clock& = delete;
  auto operator=(Clock&&) -> Clock&      = delete;
  virtual ~Clock()                       = default;

  // The low word of the tick count.
  [[nodiscard]] virtual auto Ticks() -> std::uint16_t = 0;
};

}

namespace opeu::services {
using detail::machine_clock::Clock;
}
