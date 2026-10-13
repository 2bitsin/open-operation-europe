// The shape of an equipment allocation: how many allocation rows each party takes and how many types it spreads.
#pragma once

#include <cstdint>

namespace opeu::game::detail::allocation {

struct Allocation {
  std::uint8_t slots;       // the first regiment's element slots
  std::uint8_t other_slots; // the second regiment's slots, 0 with an objective
  std::uint8_t spare_slots; // the objective's stock items the first regiment may draw
  std::uint8_t types;       // the types in the allocation
};
} // namespace opeu::game::detail::allocation

namespace opeu::game {
using detail::allocation::Allocation;
} // namespace opeu::game
