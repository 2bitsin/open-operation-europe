// Data segment DS:3eba..3eeb (50 bytes): the initialised variables of the text output (core/textout) and the
// keyboard's idle hook (io/keyboard), after the data of the file and VGA libraries; DGFONT continues at DS:3eec.
#pragma once

#include "common/data-segment.hpp"

namespace opeu::game::detail::dgtext {

// Writes the module's start-up values into its fields of the data segment (the rest of the arena is zero).
auto InitialTextState(DataSegment& data) -> void;
} // namespace opeu::game::detail::dgtext

namespace opeu::game {
using detail::dgtext::InitialTextState;
} // namespace opeu::game
