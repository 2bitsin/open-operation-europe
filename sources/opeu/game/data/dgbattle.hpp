// Data segment DS:b34e..c24f (3842 bytes). Combat, movement and casualty messages, a far handler table and far
// references to the far text segment 3c05. DS:c2d4 onward is C runtime data.
#pragma once

#include "common/data-segment.hpp"

namespace opeu::game::detail::dgbattle {

// Writes the module's start-up values into its fields of the data segment (the rest of the arena is zero).
auto InitialBattleText(DataSegment& data) -> void;
} // namespace opeu::game::detail::dgbattle

namespace opeu::game {
using detail::dgbattle::InitialBattleText;
} // namespace opeu::game
