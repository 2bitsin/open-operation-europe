// The game's busy waits: a driver or input query polled until it answers otherwise.
#pragma once

#include <concepts>

namespace opeu::game::detail::busy_wait {

// Polls `busy` until it answers false. Each poll is a driver query, which pumps the host's events.
inline auto WaitWhile(std::predicate auto busy) -> void {
  while (busy()) {
  }
}

// Polls `ready` until it answers true.
inline auto WaitUntil(std::predicate auto ready) -> void {
  while (!ready()) {
  }
}
} // namespace opeu::game::detail::busy_wait

namespace opeu::game {
using detail::busy_wait::WaitUntil;
using detail::busy_wait::WaitWhile;
} // namespace opeu::game
