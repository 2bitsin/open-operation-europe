// Data segment DS:735a..7e27 (2766 bytes). Scenario side-initializer callbacks, then dated-event text with far
// references to the far text segment 3895 and event handlers. DS:7e28 onward belongs to UITEXT.
#pragma once

#include "common/data-segment.hpp"

namespace opeu::game::detail::dgevents {

// Writes the module's start-up values into its fields of the data segment (the rest of the arena is zero).
auto InitialEventText(DataSegment& data) -> void;
} // namespace opeu::game::detail::dgevents

namespace opeu::game {
using detail::dgevents::InitialEventText;
} // namespace opeu::game
