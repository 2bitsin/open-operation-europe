// Data segment DS:27be..3e6d (5808 bytes). Unit-type, weather, side, role, terrain and order names; resource
// filenames; initial turn/presentation variables; capacity weights; person templates; the 158 shared type/event
// records and their name-pointer tables.
#pragma once

#include "common/data-segment.hpp"

namespace opeu::game::detail::dgnames {

// Writes the module's start-up values into its fields of the data segment (the rest of the arena is zero).
auto InitialNames(DataSegment& data) -> void;
} // namespace opeu::game::detail::dgnames

namespace opeu::game {
using detail::dgnames::InitialNames;
} // namespace opeu::game
