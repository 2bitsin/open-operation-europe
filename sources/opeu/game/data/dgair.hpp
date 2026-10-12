// Data segment DS:9e10..ae2d (4126 bytes). Request, air-force and special-operations messages with menu-item tables.
#pragma once

#include "common/data-segment.hpp"

namespace opeu::game::detail::dgair {

// Writes the module's start-up values into its fields of the data segment (the rest of the arena is zero).
auto InitialAirText(DataSegment& data) -> void;
} // namespace opeu::game::detail::dgair

namespace opeu::game {
using detail::dgair::InitialAirText;
} // namespace opeu::game
