// Data segment DS:7e28..7f21: request labels and commander text.
#pragma once

#include "common/data-segment.hpp"

namespace opeu::game::detail::uitext {

// Writes the module's start-up values into its fields of the data segment (the rest of the arena is zero).
auto InitialUiText(DataSegment& data) -> void;
} // namespace opeu::game::detail::uitext

namespace opeu::game {
using detail::uitext::InitialUiText;
} // namespace opeu::game
