// The native clock: the BIOS tick counter of clock.cpp.
#include "native-machine.hpp"

#include "services.hpp"

namespace opeu::services::detail::native_machine {

auto NativeClock::Ticks() -> std::uint16_t {
  return port_clock_ticks_low();
}

}
