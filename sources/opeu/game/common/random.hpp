// The game's random-number generator: a 32-bit linear congruential state (the C runtime's rand), drawn from 15 bits at a
// time. GameState owns one; the data segment keeps a copy of its state (random_state) for the saved game.
#pragma once

#include <array>
#include <bit>
#include <cstdint>

namespace opeu::game::detail::random {

using RandomState = std::array<std::uint8_t, 4>; // the state as the data segment stores it (host byte order)

class RandomGenerator {
public:
  // The next number of the sequence, 0..0x7fff.
  auto Next() noexcept -> std::uint16_t {
    _state = _state * MULTIPLIER + INCREMENT;
    return static_cast<std::uint16_t>(_state >> 16 & 0x7fff);
  }

  // A number below `bound`; a zero bound gives 0 without advancing the state.
  auto Bounded(std::uint16_t bound) noexcept -> std::uint16_t {
    if (bound == 0) return bound;
    return static_cast<std::uint16_t>(Next() % bound);
  }

  // Starts the sequence from a word; the state's high word becomes zero.
  auto Seed(std::uint16_t seed) noexcept -> void {
    _state = seed;
  }

  // Takes the state a saved game carries on.
  auto Load(RandomState const& stored) noexcept -> void {
    _state = std::bit_cast<std::uint32_t>(stored);
  }

  [[nodiscard]] auto Stored() const noexcept -> RandomState {
    return std::bit_cast<RandomState>(_state);
  }

private:
  static constexpr std::uint32_t MULTIPLIER{ 1103515245 };
  static constexpr std::uint32_t INCREMENT{ 12345 };

  std::uint32_t _state{ 0 };
};

} // namespace opeu::game::detail::random

namespace opeu::game {
using detail::random::RandomGenerator;
using detail::random::RandomState;
} // namespace opeu::game
