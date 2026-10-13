// The game's random numbers drawn from the generator state the data segment keeps (random_state, its first four
// bytes), so a saved game carries the sequence on.
#pragma once

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "core/random.hpp"

#include <cstdint>
#include <cstring>

namespace opeu::game::detail::random {

// A number in [0, bound): loads the state from the segment, draws, stores the state back.
inline auto RandomBelow(Game& game, std::uint16_t bound) -> std::uint16_t {
  auto& state{ Data(game).random_state };
  std::memcpy(&opeu_random_state, state.data(), sizeof opeu_random_state);
  auto const result{ opeu_rand_bounded(bound) };
  std::memcpy(state.data(), &opeu_random_state, sizeof opeu_random_state);
  return result;
}

// The next raw number of the sequence.
inline auto RandomNext(Game& game) -> std::uint16_t {
  auto& state{ Data(game).random_state };
  std::memcpy(&opeu_random_state, state.data(), sizeof opeu_random_state);
  auto const result{ opeu_rand_next() };
  std::memcpy(state.data(), &opeu_random_state, sizeof opeu_random_state);
  return result;
}

// Starts the sequence again from `seed`.
inline auto RandomSeed(Game& game, std::uint16_t seed) -> void {
  opeu_rand_seed(seed);
  std::memcpy(Data(game).random_state.data(), &opeu_random_state, sizeof opeu_random_state);
}
} // namespace opeu::game::detail::random

namespace opeu::game {
using detail::random::RandomBelow;
using detail::random::RandomNext;
using detail::random::RandomSeed;
} // namespace opeu::game
