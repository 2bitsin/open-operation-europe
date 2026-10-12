// Data segment DS:5184..536b (488 bytes). Startup keyboard prompt, palette/display tables and division-command text.
// DS:5184 is the startup mode byte; DS:536c..537b belongs to STEP.
#pragma once

#include "common/data-segment.hpp"

namespace opeu::game::detail::dgstart {

// Writes the module's start-up values into its fields of the data segment (the rest of the arena is zero).
auto InitialStartupText(DataSegment& data) -> void;
} // namespace opeu::game::detail::dgstart

namespace opeu::game {
using detail::dgstart::InitialStartupText;
} // namespace opeu::game
