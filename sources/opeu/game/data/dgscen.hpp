// The scenario workspace, DS:0042..2011: the scenario header, the request configurations, the sides, generals,
// groups, elements, units and objectives, and the turn cursor.
#pragma once

#include "common/data-segment.hpp"

namespace opeu::game::detail::dgscen {

// Writes the start-up image of the workspace: a loaded scenario with rebased element links. The scenario loader
// (io/loadscen) replaces it from SCEDATA and io/rebase turns the stored indices into pointers.
auto InitialWorkspace(DataSegment& data) -> void;
} // namespace opeu::game::detail::dgscen

namespace opeu::game {
using detail::dgscen::InitialWorkspace;
} // namespace opeu::game
