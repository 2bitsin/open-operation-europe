// Data segment DS:537c..5bb1 (2102 bytes). Unit/location label formats, deputy name, army-summary rows, credits,
// roster headings and city names, with their far callback pointers.
#pragma once

#include "common/data-segment.hpp"

namespace opeu::game::detail::dglabels {

// Writes the module's start-up values into its fields of the data segment (the rest of the arena is zero).
auto InitialLabels(DataSegment& data) -> void;
} // namespace opeu::game::detail::dglabels

namespace opeu::game {
using detail::dglabels::InitialLabels;
} // namespace opeu::game
