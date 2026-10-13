// Shuffling the turn order: 100 random swaps among its 64 slots, drawn from the data segment's random state.
#include "core/turnord.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"

#include <algorithm>
#include <cstdint>
#include <ranges>
#include <utility>

using namespace opeu::game;

namespace {
constexpr int SWAPS{ 100 };
} // namespace

void opeu_shuffle_turn_order(Game& game) {
  auto& order{ Data(game).turn_order };
  std::ranges::for_each(std::views::iota(0, SWAPS), [&](int) {
    auto const first{ game.State().RandomBounded(static_cast<std::uint16_t>(order.size())) };
    auto const second{ game.State().RandomBounded(static_cast<std::uint16_t>(order.size())) };
    std::swap(order[first], order[second]);
  });
}
