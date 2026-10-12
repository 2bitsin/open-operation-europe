// Data segment DS:b242..b2d3 (146 bytes). Combat messages. DS:b312..b34d is BTLCALC's module-local data and DS:b34e
// onward continues in DGBATTLE.
#pragma once

#include "common/data-segment.hpp"

namespace opeu::game::detail::dgcombat {

// Writes the module's start-up values into its fields of the data segment (the rest of the arena is zero).
auto InitialCombatText(DataSegment& data) -> void;
} // namespace opeu::game::detail::dgcombat

namespace opeu::game {
using detail::dgcombat::InitialCombatText;
} // namespace opeu::game
