// Data segment DS:3eec..44eb (1536 bytes). Proportional-display 8x16 bitmap font for characters 0x20..0x7f.
#pragma once

#include "common/data-segment.hpp"

namespace opeu::game::detail::dgfont {

// Writes the module's start-up values into its fields of the data segment (the rest of the arena is zero).
auto InitialFont(DataSegment& data) -> void;
} // namespace opeu::game::detail::dgfont

namespace opeu::game {
using detail::dgfont::InitialFont;
} // namespace opeu::game
