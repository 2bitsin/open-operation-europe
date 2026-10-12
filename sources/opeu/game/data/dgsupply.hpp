// Data segment DS:9112..9e0f (3326 bytes). Supply, victory-condition and battalion messages with menu-item tables.
#pragma once

#include "common/data-segment.hpp"

namespace opeu::game::detail::dgsupply {

// Writes the module's start-up values into its fields of the data segment (the rest of the arena is zero).
auto InitialSupplyText(DataSegment& data) -> void;
} // namespace opeu::game::detail::dgsupply

namespace opeu::game {
using detail::dgsupply::InitialSupplyText;
} // namespace opeu::game
