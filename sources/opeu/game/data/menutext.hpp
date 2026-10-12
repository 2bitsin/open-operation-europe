// DS:7f22..7f6c menu labels and DS:7fe0..806f menu descriptor. DS:7f6d..7fdf between them belongs to CTRLTEXT.
#pragma once

#include "common/data-segment.hpp"

namespace opeu::game::detail::menutext {

// Writes the module's start-up values into its fields of the data segment (the rest of the arena is zero).
auto InitialMenuText(DataSegment& data) -> void;
} // namespace opeu::game::detail::menutext

namespace opeu::game {
using detail::menutext::InitialMenuText;
} // namespace opeu::game
