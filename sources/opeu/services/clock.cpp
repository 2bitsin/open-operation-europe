// BIOS clock (INT 1Ah AH=00h) and the C runtime's console read (getch). (INT 21h AH=0Dh disk reset lives in fileio.cpp next to the
// handle table.)
#include "hostinput.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cinttypes>
#include <cstdint>
#include <ctime>
#include <optional>
#include <utility>

namespace opeu::services::detail::clock {
namespace {

constexpr auto US_PER_SECOND{ std::uint64_t{ 1'000'000 } };
constexpr auto PIT_HZ{ std::uint64_t{ 1'193'182 } };

// Seconds since the local midnight. std::localtime and not the chrono zone database, which the browser build lacks.
[[nodiscard]] auto LocalSecondsOfDay() -> std::optional<std::uint64_t> {
  auto const        now{ std::time(nullptr) };
  auto const* const local{ std::localtime(&now) };
  if (local == nullptr) return std::nullopt;
  return static_cast<std::uint64_t>(local->tm_hour * 3600 + local->tm_min * 60 + local->tm_sec);
}

// MS C's console read (getch) over the BIOS keyboard: a key with ASCII 0 returns 0 now and its scan code on the next call.
class Getch {
public:
  [[nodiscard]] auto Next() -> int {
    if (_scan_pending) return *std::exchange(_scan_pending, std::nullopt);
    auto const key{ port_key_read() };
    auto const ascii{ key & 0xFF };
    if (ascii == 0) _scan_pending = key >> 8;
    return ascii;
  }

private:
  std::optional<int> _scan_pending;
};

// The getch state of the process (legacy singleton, listed for B5).
auto TheGetch() -> Getch& {
  static Getch getch;
  return getch;
}

}
}

// DX of INT 1Ah AH=0: low word of the timer ticks since midnight (1193182/65536 = 18.2065 Hz). Window runs count from
// the local midnight; headless runs from virtual time 0, so they are reproducible.
auto port_clock_ticks_low() -> uint16_t {
  using namespace opeu::services::detail::clock;
  auto       us{ port_time_us() };
  auto const seconds{ port_options.headless ? std::nullopt : LocalSecondsOfDay() };
  if (seconds) us = *seconds * US_PER_SECOND + us % US_PER_SECOND;
  auto const ticks{ static_cast<std::uint32_t>(us * PIT_HZ / 65536 / US_PER_SECOND & 0xFFFF) };
  port_trace("svc", "int1a ticks -> %04" PRIx32, ticks);
  return static_cast<std::uint16_t>(ticks);
}

// MS C's console read (getch): INT 21h AH=08h, the ASCII of the next key; a key with ASCII 0 returns 0 and its scan code on the next
// call.
auto port_getch() -> int {
  return opeu::services::detail::clock::TheGetch().Next();
}
