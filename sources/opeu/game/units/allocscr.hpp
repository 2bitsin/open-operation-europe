// The equipment allocation screen.
#pragma once

#include "common/events.hpp"
#include "types.hpp"
#include "units/allocation.hpp"

#include <cstdint>

namespace opeu::game::detail::allocscr {

// An amount of one type moved into a stock entry.
struct Take {
  std::uint16_t entry; // the stock entry (an Item) that receives it
  std::uint8_t  slot;  // the type's row
  std::uint16_t amount;
};
} // namespace opeu::game::detail::allocscr

namespace opeu::game {
using detail::allocscr::Take;
} // namespace opeu::game

void opeu_alloc_amounts_show(Game& game, uint8_t count);
void opeu_alloc_take(Game& game, opeu::game::Take const& take, uint8_t count);
void opeu_alloc_screen(Game& game, opeu::game::Allocation const& allocation);
