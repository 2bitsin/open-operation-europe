// Shuffling the turn order: 100 random swaps among its 64 slots.
#include "core/turnord.hpp"
#include "core/random.hpp"
#include "data/mapstate.hpp"
#include "types.hpp"

#include <utility>

void opeu_shuffle_turn_order(void) {
  for (auto swap = 0; swap < 100; ++swap) {
    auto& first{ opeu_turn_order[opeu_rand_bounded(64)] };
    auto& second{ opeu_turn_order[opeu_rand_bounded(64)] };
    std::swap(first, second);
  }
}
