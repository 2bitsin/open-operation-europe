// The state changes of the day's turn points: the generals' counters run down, the sides' pools take in what is
// pending and their income.
#include "core/periodic.hpp"
#include "common/game.hpp"
#include "core/counter.hpp"
#include "core/numeric.hpp"
#include "data-segment.hpp"
#include "types.hpp"

using namespace opeu::game;

// Counts down every general's counter that is not yet zero.
void opeu_decrement_record_counters(Game& game) {
  for (auto& person : Data(game).people) {
    if (auto count{ opeu_get_record_counter(person) }) opeu_set_record_counter(person, --count);
  }
}

// Readies each side's air cargo flight and airborne drop for the day and books its pending reserves and planes and ten times its income into its pools, each capped.
void opeu_apply_daily_side_resources(Game& game) {
  auto& data{ Data(game) };
  for (uint8_t a = 0; a < SIDE_COUNT; ++a) {
    auto& side{ data.sides[a] };
    data.side_flags[a] |= Bits(SideFlag::AIR_CARGO_READY) | Bits(SideFlag::AIR_DROP_READY);
    opeu_add_word(side.pending_bombers, 9999, &side.bombers);
    side.pending_bombers = 0;
    opeu_add_word(side.pending_planes, 999, &side.planes);
    side.pending_planes = 0;
    // The income times ten wraps to a word before the saturating addition.
    for (auto i = 0; i < 3; ++i) opeu_add_word(static_cast<uint16_t>(side.income[i] * 10u), 60000, &side.stock[i]);
  }
}
