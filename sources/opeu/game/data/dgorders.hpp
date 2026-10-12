// Data segment DS:8396..9111 (3452 bytes). Unit order, report and assignment messages with 12-byte menu-item tables.
#pragma once

#include "common/data-segment.hpp"

namespace opeu::game::detail::dgorders {

// Writes the module's start-up values into its fields of the data segment (the rest of the arena is zero).
auto InitialOrderText(DataSegment& data) -> void;
} // namespace opeu::game::detail::dgorders

namespace opeu::game {
using detail::dgorders::InitialOrderText;
} // namespace opeu::game
