// BIOS clock (INT 1Ah AH=00h) and the C runtime's console read (getch). (INT 21h AH=0Dh disk reset lives in fileio.cpp next to the
// handle table.)
#include "console-read.hpp"
#include "hostinput.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cinttypes>
#include <cstdint>
#include <ctime>
#include <optional>

namespace opeu::services::detail::clock {
namespace {

constexpr auto US_PER_SECOND{ std::uint64_t{ 1'000'000 } };
constexpr auto PIT_HZ{ std::uint64_t{ 1'193'182 } };

// Seconds since the local midnight. std::localtime and not the chrono zone database, which the browser build lacks.
[[nodiscard]] auto LocalSecondsOfDay() -> std::optional<std::uint64_t> {
  auto const        now{ std::time(nullptr) };
  auto const* const local{ std::localtime(&now) };
  if (local == nullptr) return std::nullopt;
  return static_cast<std::uint64_t>((local->tm_hour * 3600) + (local->tm_min * 60) + local->tm_sec);
}

// The low word of the BIOS timer ticks (1193182/65536 = 18.2065 Hz) at `us` microseconds of host time; with the
// seconds since the local midnight, counted from that midnight.
[[nodiscard]] auto TicksLow(std::uint64_t us, std::optional<std::uint64_t> const seconds_of_day) -> std::uint16_t {
  if (seconds_of_day) us = (*seconds_of_day * US_PER_SECOND) + (us % US_PER_SECOND);
  return static_cast<std::uint16_t>((us * PIT_HZ / 65536 / US_PER_SECOND) & 0xFFFF);
}

// The getch state of the process (legacy singleton, listed for B5).
auto TheConsoleRead() -> ConsoleRead& {
  static ConsoleRead console;
  return console;
}

}
}

// DX of INT 1Ah AH=0: low word of the timer ticks since midnight (1193182/65536 = 18.2065 Hz). Window runs count from
// the local midnight; headless runs from virtual time 0, so they are reproducible.
auto port_clock_ticks_low() -> uint16_t {
  using namespace opeu::services::detail::clock;
  auto const ticks{ TicksLow(port_time_us(), (port_options.headless != 0) ? std::nullopt : LocalSecondsOfDay()) };
  port_trace("svc", "int1a ticks -> %04" PRIx32, std::uint32_t{ ticks });
  return ticks;
}

// MS C's console read (getch): INT 21h AH=08h, the ASCII of the next key; a key with ASCII 0 returns 0 and its scan code on the next
// call.
auto port_getch() -> int {
  return opeu::services::detail::clock::TheConsoleRead().Next(port_key_read);
}
