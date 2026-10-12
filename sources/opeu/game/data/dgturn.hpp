// Data segment DS:8070..838d (798 bytes). Direction keys, turn/order report formats and unit status text.
// DS:838e..8395 belongs to SELECT.
#pragma once

#include "common/data-segment.hpp"

namespace opeu::game::detail::dgturn {

// Writes the module's start-up values into its fields of the data segment (the rest of the arena is zero).
auto InitialTurnText(DataSegment& data) -> void;
} // namespace opeu::game::detail::dgturn

namespace opeu::game {
using detail::dgturn::InitialTurnText;
} // namespace opeu::game
