// The interactive map location picker (map/mappick.cpp).
#pragma once

#include "common/events.hpp"
#include "common/grid-cell.hpp"

#include <optional>

// The cell the player picks on the map, the cursor starting on `start`; none when the player cancels.
[[nodiscard]] auto PickLocation(Game& game, opeu::game::GridCell start) -> std::optional<opeu::game::GridCell>;
