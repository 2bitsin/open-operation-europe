// Data segment DS:5bb2..7359 (6056 bytes). Scenario briefing and commander speech texts.
#pragma once

#include "common/data-segment.hpp"

namespace opeu::game::detail::dgbrief {

// Writes the module's start-up values into its fields of the data segment (the rest of the arena is zero).
auto InitialBriefingText(DataSegment& data) -> void;
} // namespace opeu::game::detail::dgbrief

namespace opeu::game {
using detail::dgbrief::InitialBriefingText;
} // namespace opeu::game
