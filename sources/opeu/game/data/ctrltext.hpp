// DS:7f6d..7fdf: command prompts and eleven far action pointers.
#pragma once

#include "common/data-segment.hpp"

namespace opeu::game::detail::ctrltext {

// Writes the module's start-up values into its fields of the data segment (the rest of the arena is zero).
auto InitialControlText(DataSegment& data) -> void;
} // namespace opeu::game::detail::ctrltext

namespace opeu::game {
using detail::ctrltext::InitialControlText;
} // namespace opeu::game
