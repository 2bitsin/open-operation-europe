// Casualties for every element of a regiment.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

namespace opeu::game::detail::casualty {

// The losses a regiment's elements roll: `chance` percent of them lose up to spread + base men (the others up to
// spread / 2); heavy multiplies infantry's losses by 6 to 15; blast shows the explosion where a player sees it.
struct CasualtyRoll {
  std::uint16_t chance;
  std::uint16_t base;
  std::uint16_t spread;
  bool          heavy;
  bool          blast;
};
} // namespace opeu::game::detail::casualty

namespace opeu::game {
using detail::casualty::CasualtyRoll;
} // namespace opeu::game

void opeu_unit_casualties(Game& game, uint16_t unit, opeu::game::CasualtyRoll const& roll);
