// Data segment DS:44ec..5176 (3211 bytes). Setup-screen data: far callbacks, save/load prompts, general selection,
// ability editing, game-option menus and scenario titles. DS:5177..5183 belongs to the calendar module.
#pragma once

#include "common/data-segment.hpp"

namespace opeu::game::detail::dgsetup {

// Writes the module's start-up values into its fields of the data segment (the rest of the arena is zero).
auto InitialSetupText(DataSegment& data) -> void;
} // namespace opeu::game::detail::dgsetup

namespace opeu::game {
using detail::dgsetup::InitialSetupText;
} // namespace opeu::game
